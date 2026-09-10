#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "dsp/SynthEngine.h"
#include "presets/PresetManager.h"
#include "state/MutationEngine.h"
#include "dsp/fracture/Fragment.h"
#include "state/ModRouting.h"
#include "dsp/fx/SpacePresets.h"

namespace am
{

/**
    The AudioProcessor. Owns the host parameter tree, the engine and the
    preset/state machinery. Keeps the audio callback free of anything that
    is not the engine.
*/
class AntiMatrProcessor final : public juce::AudioProcessor,
                                public juce::ChangeBroadcaster,
                                private juce::Timer,
                                private juce::AudioProcessorValueTreeState::Listener,
                                private juce::AsyncUpdater
{
public:
    AntiMatrProcessor();
    ~AntiMatrProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ANTI-MATR"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& parameters() noexcept { return apvts; }
    SynthEngine& engine() noexcept { return synth; }
    Diagnostics& diagnostics() noexcept { return synth.diagnostics(); }
    PresetManager& presets() noexcept { return presetManager; }
    juce::MidiKeyboardState& keyboardState() noexcept { return keyboard; }

    /** Thread-safe MIDI injection used by DSP LAB stress tests and tools (any thread). */
    void injectMidi (const juce::MidiMessage& m) { devMidi.addMessageToQueue (m); }

    /** Current patch built from the live parameter values (message thread). */
    PatchState currentPatch() const;

    /** Applies a patch (message thread). Parameters are pushed through the host tree. */
    void loadPatch (const PatchState& patch, bool notifyPresetChange = true);

    void loadFactoryPreset (int index);
    void loadNextPreset (int direction);
    void loadRandomPreset();
    int  currentPresetIndex() const noexcept { return currentPreset; }
    juce::String currentPresetName() const { return presetName; }
    juce::StringArray currentPresetTags() const { return presetTags; }

    /** Top-bar MUTATE: deterministic per click (the seed advances). */
    void mutate (MutationStrength strength);
    /** Top-bar RANDOM: a new random-but-safe patch. */
    void randomizePatch();

    /** A/B slots. */
    int  currentABSlot() const noexcept { return abSlot; }
    void selectABSlot (int slot);
    void copyABToOther();

    /** Diagnostics-friendly parameter snapshot (message thread). */
    ParamValues currentParamValues() const;

    /** Fracture fragment / sequencer table (message thread). Setting it publishes to the engine. */
    const FractureTable& getFractureTable() const noexcept { return fractureTable; }
    void setFractureTable (const FractureTable& table);

    /** Modulation matrix (message thread). Setting it publishes to the engine. */
    const ModRoutingTable& getModRoutings() const noexcept { return modRoutings; }
    void setModRoutings (const ModRoutingTable& routings);

    static constexpr int kDefaultWidth  = 1600;
    static constexpr int kDefaultHeight = 1000;
    int lastEditorWidth  = kDefaultWidth;
    int lastEditorHeight = kDefaultHeight;

private:
    void timerCallback() override { synth.messageThreadMaintenance(); }
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void applySpacePreset (int type);
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void snapshotParameters (ParamValues& out) const noexcept;
    void markPreset (const juce::String& name, const juce::StringArray& tags, int index);

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>*, kNumParams> rawValues {};

    SynthEngine synth;
    PresetManager presetManager;
    juce::MidiKeyboardState keyboard;
    juce::MidiMessageCollector devMidi;

    ParamValues blockParams {};
    int currentPreset = 0;
    juce::String presetName = "Init";
    juce::StringArray presetTags;
    uint32_t mutationSeed = 1;
    uint32_t randomSeed = 1000;

    int abSlot = 0;
    PatchState abStates[2];

    int reportedLatency = 0;
    PatchState extraState;   ///< non-parameter sections kept for round-tripping
    FractureTable fractureTable = FractureTable::makeDefault();
    ModRoutingTable modRoutings;
    std::atomic<int> pendingSpaceType { -1 };
    bool suppressSpaceRecall = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AntiMatrProcessor)
};

} // namespace am
