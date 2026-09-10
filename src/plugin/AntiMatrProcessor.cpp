#include "AntiMatrProcessor.h"
#include "AntiMatrEditor.h"
#include "state/StateManager.h"

namespace am
{

namespace
{
    const juce::Identifier kParametersType ("PARAMETERS");
}

AntiMatrProcessor::AntiMatrProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, kParametersType, createParameterLayout())
{
    jassert (ParameterRegistry::validate().isEmpty());

    for (const auto& d : ParameterRegistry::all())
        rawValues[(size_t) paramIndex (d.param)] = apvts.getRawParameterValue (d.id);

    ParameterRegistry::fillDefaults (blockParams);
    abStates[0] = abStates[1] = PresetManager::initPatch();
    markPreset ("Init", { "basic" }, 0);
    synth.fractureEngine().publishTable (std::make_unique<FractureTable> (fractureTable));

    // The default parameter values become the curated NEBULA rack, then follow the Space picker.
    {
        ParamValues v = currentParamValues();
        SpacePresets::apply (paramChoice (v, Param::spaceType), v);
        suppressSpaceRecall = true;
        apvts.replaceState (StateManager::toParameterTree (v, kParametersType));
        suppressSpaceRecall = false;
        abStates[0] = abStates[1] = currentPatch();
    }
    apvts.addParameterListener (ParameterRegistry::get (Param::spaceType).id, this);
    startTimer (500);
}

void AntiMatrProcessor::parameterChanged (const juce::String&, float newValue)
{
    // May arrive on the audio thread (automation): defer the rack recall to the message thread.
    if (suppressSpaceRecall) return;
    pendingSpaceType.store ((int) std::lround (newValue), std::memory_order_relaxed);
    triggerAsyncUpdate();
}

void AntiMatrProcessor::handleAsyncUpdate()
{
    const int type = pendingSpaceType.exchange (-1, std::memory_order_relaxed);
    if (type >= 0) applySpacePreset (type);
}

void AntiMatrProcessor::applySpacePreset (int type)
{
    ParamValues before = currentParamValues();
    ParamValues after = before;
    SpacePresets::apply (type, after);
    for (const auto& d : ParameterRegistry::all())
    {
        const size_t i = (size_t) paramIndex (d.param);
        if (after[i] == before[i]) continue;
        if (auto* p = apvts.getParameter (d.id))
            p->setValueNotifyingHost (p->convertTo0to1 (after[i]));
    }
}

void AntiMatrProcessor::setFractureTable (const FractureTable& table)
{
    fractureTable = table;
    synth.fractureEngine().publishTable (std::make_unique<FractureTable> (fractureTable));
}

AntiMatrProcessor::~AntiMatrProcessor()
{
    stopTimer();
    cancelPendingUpdate();
    apvts.removeParameterListener (ParameterRegistry::get (Param::spaceType).id, this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AntiMatrProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& d : ParameterRegistry::all())
    {
        const juce::ParameterID pid (d.id, 1);
        const ParamDesc* desc = &d;

        switch (d.kind)
        {
            case ParamKind::Float:
            {
                juce::NormalisableRange<float> range (d.min, d.max, 0.0f, d.skew);
                auto attrs = juce::AudioParameterFloatAttributes()
                                .withLabel (d.unit)
                                .withStringFromValueFunction ([desc] (float v, int) { return desc->formatValue (v); });
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, range, d.defaultValue, attrs));
                break;
            }
            case ParamKind::Int:
            {
                auto attrs = juce::AudioParameterIntAttributes().withLabel (d.unit);
                layout.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.defaultValue, attrs));
                break;
            }
            case ParamKind::Bool:
            {
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.defaultValue >= 0.5f));
                break;
            }
            case ParamKind::Choice:
            {
                juce::StringArray items;
                items.addTokens (juce::String (d.choices), "|", "");
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, items, (int) d.defaultValue));
                break;
            }
        }
    }
    return layout;
}

void AntiMatrProcessor::snapshotParameters (ParamValues& out) const noexcept
{
    for (size_t i = 0; i < (size_t) kNumParams; ++i)
        out[i] = rawValues[i] != nullptr ? rawValues[i]->load (std::memory_order_relaxed) : out[i];
}

ParamValues AntiMatrProcessor::currentParamValues() const
{
    ParamValues v {};
    ParameterRegistry::fillDefaults (v);
    snapshotParameters (v);
    return v;
}

//==============================================================================
void AntiMatrProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.prepare (sampleRate, samplesPerBlock);
    devMidi.reset (sampleRate);
    snapshotParameters (blockParams);
    synth.control().resetTo (blockParams);
    reportedLatency = synth.latencySamples();
    setLatencySamples (reportedLatency);
}

void AntiMatrProcessor::releaseResources()
{
}

bool AntiMatrProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void AntiMatrProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    keyboard.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    devMidi.removeNextBlockOfMessages (midi, buffer.getNumSamples());

    snapshotParameters (blockParams);

    TransportInfo transport;
    if (auto* ph = getPlayHead())
    {
        if (const auto pos = ph->getPosition())
        {
            if (const auto bpm = pos->getBpm()) transport.bpm = *bpm;
            if (const auto ppq = pos->getPpqPosition()) transport.ppqPosition = *ppq;
            if (const auto sig = pos->getTimeSignature()) { transport.timeSigNum = sig->numerator; transport.timeSigDen = sig->denominator; }
            transport.isPlaying = pos->getIsPlaying();
        }
    }

    // Render into a stereo view of the buffer; mono hosts receive the left channel.
    if (buffer.getNumChannels() >= 2)
    {
        synth.process (buffer, midi, blockParams, transport);
    }
    else if (buffer.getNumChannels() == 1)
    {
        juce::AudioBuffer<float> stereo (2, buffer.getNumSamples());
        synth.process (stereo, midi, blockParams, transport);
        buffer.copyFrom (0, 0, stereo, 0, 0, buffer.getNumSamples());
        buffer.addFrom (0, 0, stereo, 1, 0, buffer.getNumSamples());
        buffer.applyGain (0.5f);
    }

    midi.clear();

    const int latency = synth.latencySamples();
    if (latency != reportedLatency)
    {
        reportedLatency = latency;
        setLatencySamples (latency);
    }
}

//==============================================================================
juce::AudioProcessorEditor* AntiMatrProcessor::createEditor()
{
    return new AntiMatrEditor (*this);
}

//==============================================================================
int AntiMatrProcessor::getNumPrograms() { return std::max (1, presetManager.numFactoryPresets()); }
int AntiMatrProcessor::getCurrentProgram() { return currentPreset; }
void AntiMatrProcessor::setCurrentProgram (int index) { loadFactoryPreset (index); }
const juce::String AntiMatrProcessor::getProgramName (int index)
{
    return index >= 0 && index < presetManager.numFactoryPresets() ? presetManager.factoryPreset (index).name : juce::String ("Init");
}

//==============================================================================
PatchState AntiMatrProcessor::currentPatch() const
{
    PatchState s = extraState;
    s.params = currentParamValues();
    s.fracture = fractureTable.toVar();
    s.meta.name = presetName;
    s.meta.tags = presetTags;
    s.meta.pluginVersion = ANTIMATR_VERSION_STRING;
    return s;
}

void AntiMatrProcessor::markPreset (const juce::String& name, const juce::StringArray& tags, int index)
{
    presetName = name;
    presetTags = tags;
    currentPreset = index;
}

void AntiMatrProcessor::loadPatch (const PatchState& patch, bool notifyPresetChange)
{
    extraState = patch;
    fractureTable = patch.fracture.isVoid() ? FractureTable::makeDefault() : FractureTable::fromVar (patch.fracture);
    synth.fractureEngine().publishTable (std::make_unique<FractureTable> (fractureTable));
    suppressSpaceRecall = true;    // a patch carries its own rack values
    apvts.replaceState (StateManager::toParameterTree (patch.params, kParametersType));
    suppressSpaceRecall = false;
    markPreset (patch.meta.name, patch.meta.tags, presetManager.findFactory (patch.meta.name));
    diagnostics().events.push (EngineEventType::PresetLoaded, Subsystem::State, -1, (uint32_t) std::max (0, currentPreset), 0.0f,
                               diagnostics().sampleClock.load());
    if (notifyPresetChange)
        sendChangeMessage();
}

void AntiMatrProcessor::loadFactoryPreset (int index)
{
    if (presetManager.numFactoryPresets() == 0) return;
    index = ((index % presetManager.numFactoryPresets()) + presetManager.numFactoryPresets()) % presetManager.numFactoryPresets();
    loadPatch (presetManager.buildFactory (index));
    currentPreset = index;
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void AntiMatrProcessor::loadNextPreset (int direction)
{
    loadFactoryPreset (currentPreset + (direction >= 0 ? 1 : -1));
}

void AntiMatrProcessor::loadRandomPreset()
{
    randomizePatch();
}

void AntiMatrProcessor::mutate (MutationStrength strength)
{
    PatchState s = currentPatch();
    const int typeBefore = paramChoice (s.params, Param::spaceType);
    MutationEngine::mutate (s.params, strength, mutationSeed++);
    if (paramChoice (s.params, Param::spaceType) != typeBefore)
        SpacePresets::apply (paramChoice (s.params, Param::spaceType), s.params);
    s.meta.name = presetName.endsWith ("*") ? presetName : presetName + " *";
    loadPatch (s);
}

void AntiMatrProcessor::randomizePatch()
{
    PatchState s = currentPatch();
    MutationEngine::randomize (s.params, randomSeed++);
    SpacePresets::apply (paramChoice (s.params, Param::spaceType), s.params);
    s.meta.name = "Random " + juce::String (randomSeed - 1000);
    s.meta.tags = { "random" };
    loadPatch (s);
}

void AntiMatrProcessor::selectABSlot (int slot)
{
    slot = juce::jlimit (0, 1, slot);
    if (slot == abSlot) return;
    abStates[abSlot] = currentPatch();
    abSlot = slot;
    loadPatch (abStates[abSlot]);
}

void AntiMatrProcessor::copyABToOther()
{
    abStates[1 - abSlot] = currentPatch();
}

//==============================================================================
void AntiMatrProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    PatchState s = currentPatch();
    auto* ui = new juce::DynamicObject();
    ui->setProperty ("width", lastEditorWidth);
    ui->setProperty ("height", lastEditorHeight);
    ui->setProperty ("abSlot", abSlot);
    s.ui = juce::var (ui);

    // Embed the other A/B slot so a session reload keeps both.
    auto* ab = new juce::DynamicObject();
    ab->setProperty ("other", StateManager::toVar (abStates[1 - abSlot]));
    s.ab = juce::var (ab);

    destData = StateManager::toBinary (s);
}

void AntiMatrProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    PatchState s;
    juce::String warnings;
    if (! StateManager::fromBinary (data, (size_t) sizeInBytes, s, &warnings))
        return;

    if (auto* ui = s.ui.getDynamicObject())
    {
        lastEditorWidth  = juce::jmax (100, (int) ui->getProperty ("width"));
        lastEditorHeight = juce::jmax (100, (int) ui->getProperty ("height"));
        abSlot = juce::jlimit (0, 1, (int) ui->getProperty ("abSlot"));
    }
    if (auto* ab = s.ab.getDynamicObject())
    {
        PatchState other;
        if (StateManager::fromVar (ab->getProperty ("other"), other))
            abStates[1 - abSlot] = other;
    }
    s.ab = juce::var();
    s.ui = juce::var();
    loadPatch (s);
    abStates[abSlot] = s;
}

} // namespace am
