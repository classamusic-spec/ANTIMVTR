#include "VoiceManager.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

VoiceManager::VoiceManager()
{
    voices.resize (kMaxVoices);
}

void VoiceManager::prepare (double sampleRate, int maxBlockSize, Diagnostics* diagnostics)
{
    sr = sampleRate;
    diag = diagnostics;
    for (auto& v : voices)
        v.prepare (sampleRate, maxBlockSize);
    reset();
}

void VoiceManager::reset()
{
    for (auto& v : voices) v.reset();
    channelBend.fill (0.0f);
    channelPressure.fill (0.0f);
    modWheel = 0.0f;
    sustainDown = false;
    numHeld = 0;
    lastStartedVoice = -1;
    lastMonoFrequency = 0.0;
    // The note counter seeds every per-note random stream in the instrument — wave start phase,
    // dust, the impact strike, gesture noise, sample grain positions, Evolve scatter. Carrying it
    // across a reset means the first note of a newly loaded patch is coloured by how many notes the
    // previous patch happened to play.
    noteCounter = 0;
    heldNotes.fill (0);
    heldVelocities.fill (0.0f);
}

void VoiceManager::setMaxVoices (int n) noexcept
{
    n = juce::jlimit (1, kMaxVoices, n);
    if (n == maxVoices) return;
    maxVoices = n;
    for (int i = n; i < kMaxVoices; ++i)
        if (voices[(size_t) i].isActive()) voices[(size_t) i].kill();
    if (diag != nullptr)
        diag->events.push (EngineEventType::PolyphonyChanged, Subsystem::VoiceMix, -1, (uint32_t) n, 0.0f,
                           diag->sampleClock.load (std::memory_order_relaxed));
}

int VoiceManager::activeVoiceCount() const noexcept
{
    int c = 0;
    for (int i = 0; i < maxVoices; ++i)
        if (voices[(size_t) i].isActive()) ++c;
    return c;
}

void VoiceManager::pushHeld (int note, float vel) noexcept
{
    removeHeld (note);
    if (numHeld >= kMaxHeld)
    {
        for (int i = 1; i < kMaxHeld; ++i) { heldNotes[(size_t) i - 1] = heldNotes[(size_t) i]; heldVelocities[(size_t) i - 1] = heldVelocities[(size_t) i]; }
        numHeld = kMaxHeld - 1;
    }
    heldNotes[(size_t) numHeld] = note;
    heldVelocities[(size_t) numHeld] = vel;
    ++numHeld;
}

void VoiceManager::removeHeld (int note) noexcept
{
    for (int i = 0; i < numHeld; ++i)
    {
        if (heldNotes[(size_t) i] == note)
        {
            for (int j = i + 1; j < numHeld; ++j) { heldNotes[(size_t) j - 1] = heldNotes[(size_t) j]; heldVelocities[(size_t) j - 1] = heldVelocities[(size_t) j]; }
            --numHeld;
            return;
        }
    }
}

float VoiceManager::bendSemisFor (int channel, const ParamValues& params) const noexcept
{
    const float range = paramValue (params, Param::masterBendRange);
    const int ch = juce::jlimit (0, 16, channel);
    return channelBend[(size_t) ch] * range;
}

int VoiceManager::findFreeVoice() const noexcept
{
    for (int i = 0; i < maxVoices; ++i)
        if (! voices[(size_t) i].isActive()) return i;
    return -1;
}

int VoiceManager::stealVoice() noexcept
{
    // Prefer the quietest releasing voice; otherwise the oldest voice.
    int best = -1;
    float bestScore = std::numeric_limits<float>::max();
    for (int i = 0; i < maxVoices; ++i)
    {
        const auto& v = voices[(size_t) i];
        const float ageScore = (float) v.noteId();               // lower = older
        const float releasing = v.isReleasing() ? 0.0f : 1.0e9f;  // releasing voices first
        const float score = releasing + v.envelopeLevel() * 1.0e6f + ageScore;
        if (score < bestScore) { bestScore = score; best = i; }
    }
    if (best >= 0 && diag != nullptr)
        diag->events.push (EngineEventType::VoiceStolen, Subsystem::VoiceMix, best, (uint32_t) voices[(size_t) best].midiNote(), 0.0f,
                           diag->sampleClock.load (std::memory_order_relaxed));
    return best;
}

int VoiceManager::findVoiceForNote (int channel, int midiNote) const noexcept
{
    int best = -1;
    uint32_t newest = 0;
    for (int i = 0; i < maxVoices; ++i)
    {
        const auto& v = voices[(size_t) i];
        if (v.isActive() && v.isGated() && v.midiNote() == midiNote && (channel == 0 || v.channel() == channel))
            if (v.noteId() >= newest) { newest = v.noteId(); best = i; }
    }
    return best;
}

void VoiceManager::noteOn (int channel, int midiNote, float velocity, const ParamValues& params, Quality quality)
{
    const int mode = paramChoice (params, Param::masterMode); // 0 poly, 1 mono, 2 legato
    const double glide = (double) paramValue (params, Param::masterGlide);

    AntiMatrVoice::StartInfo info;
    info.midiNote = midiNote;
    info.velocity = velocity;
    info.channel  = channel;
    info.noteId   = ++noteCounter;
    info.pitchBendSemis = bendSemisFor (channel, params);
    info.pressure = channelPressure[(size_t) juce::jlimit (0, 16, channel)];
    info.modWheel = modWheel;
    info.quality  = quality;

    if (mode == 0)
    {
        int idx = findFreeVoice();
        if (idx < 0) idx = stealVoice();
        if (idx < 0) return;

        // Poly glide: from the most recently started voice.
        if (glide > 0.0005 && lastStartedVoice >= 0)
        {
            info.glideSeconds = glide;
            info.fromFrequency = voices[(size_t) lastStartedVoice].noteState().baseFrequency;
        }

        voices[(size_t) idx].start (info, params);
        lastStartedVoice = idx;
    }
    else
    {
        // MONO / LEGATO: a single voice (index 0).
        const bool wasHeld = numHeld > 0;
        pushHeld (midiNote, velocity);
        auto& v = voices[0];
        const bool voiceRunning = v.isActive() && v.isGated();

        info.legato = (mode == 2) && voiceRunning;
        if (glide > 0.0005 && (voiceRunning || wasHeld) && lastMonoFrequency > 0.0)
        {
            info.glideSeconds = glide;
            info.fromFrequency = v.isActive() ? v.noteState().frequency : lastMonoFrequency;
        }

        // Other voices (from a previous poly mode) fade out.
        for (int i = 1; i < maxVoices; ++i)
            if (voices[(size_t) i].isActive()) voices[(size_t) i].kill();

        v.start (info, params);
        lastStartedVoice = 0;
        lastMonoFrequency = v.noteState().baseFrequency;
    }

    if (diag != nullptr)
        diag->events.push (EngineEventType::VoiceStarted, Subsystem::VoiceMix, lastStartedVoice, (uint32_t) midiNote, velocity,
                           diag->sampleClock.load (std::memory_order_relaxed));
}

void VoiceManager::noteOff (int channel, int midiNote, const ParamValues& params, Quality quality)
{
    const int mode = paramChoice (params, Param::masterMode);

    if (mode == 0)
    {
        for (int i = 0; i < maxVoices; ++i)
        {
            auto& v = voices[(size_t) i];
            if (v.isActive() && v.isGated() && v.midiNote() == midiNote && (channel == 0 || v.channel() == channel))
            {
                if (sustainDown) v.setSustained (true);   // keeps sounding until the pedal is lifted
                else             v.release();
            }
        }
        return;
    }

    // MONO / LEGATO
    removeHeld (midiNote);
    auto& v = voices[0];
    if (! v.isActive() || v.midiNote() != midiNote) return;

    if (numHeld > 0)
    {
        // Return to the previously held key.
        AntiMatrVoice::StartInfo info;
        info.midiNote = heldNotes[(size_t) numHeld - 1];
        info.velocity = heldVelocities[(size_t) numHeld - 1];
        info.channel  = channel;
        info.noteId   = ++noteCounter;
        info.legato   = (mode == 2);
        info.pitchBendSemis = bendSemisFor (channel, params);
        info.quality  = quality;
        const double glide = (double) paramValue (params, Param::masterGlide);
        if (glide > 0.0005) { info.glideSeconds = glide; info.fromFrequency = v.noteState().frequency; }
        v.start (info, params);
        lastMonoFrequency = v.noteState().baseFrequency;
    }
    else if (sustainDown)
    {
        v.setSustained (true);
    }
    else
    {
        v.release();
    }
}

void VoiceManager::handleMidi (const juce::MidiMessage& m, const ParamValues& params, Quality quality)
{
    const int ch = juce::jlimit (0, 16, m.getChannel());

    if (m.isNoteOn())
    {
        noteOn (ch, m.getNoteNumber(), m.getFloatVelocity(), params, quality);
    }
    else if (m.isNoteOff())
    {
        noteOff (ch, m.getNoteNumber(), params, quality);
    }
    else if (m.isPitchWheel())
    {
        const float bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        channelBend[(size_t) ch] = bend;
        for (int i = 0; i < maxVoices; ++i)
        {
            auto& v = voices[(size_t) i];
            if (v.isActive() && (v.channel() == ch || ch == 0))
                v.setPitchBend (bend * paramValue (params, Param::masterBendRange));
        }
    }
    else if (m.isChannelPressure())
    {
        const float p = (float) m.getChannelPressureValue() / 127.0f;
        channelPressure[(size_t) ch] = p;
        for (int i = 0; i < maxVoices; ++i)
        {
            auto& v = voices[(size_t) i];
            if (v.isActive() && (v.channel() == ch || ch == 0)) v.setPressure (p);
        }
    }
    else if (m.isAftertouch())
    {
        const float p = (float) m.getAfterTouchValue() / 127.0f;
        for (int i = 0; i < maxVoices; ++i)
        {
            auto& v = voices[(size_t) i];
            if (v.isActive() && v.midiNote() == m.getNoteNumber()) v.setPressure (p);
        }
    }
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        const float value = (float) m.getControllerValue() / 127.0f;
        if (cc == 1)
        {
            modWheel = value;
            for (int i = 0; i < maxVoices; ++i)
                if (voices[(size_t) i].isActive()) voices[(size_t) i].setModWheel (value);
        }
        else if (cc == 64)
        {
            const bool down = m.getControllerValue() >= 64;
            if (sustainDown && ! down)
            {
                // Pedal up: release every voice that was held only by the pedal.
                for (int i = 0; i < maxVoices; ++i)
                {
                    auto& v = voices[(size_t) i];
                    if (v.isActive() && v.noteState().sustained)
                        v.release();
                }
            }
            sustainDown = down;
        }
        else if (cc == 120 || cc == 123)
        {
            allNotesOff (cc == 120);
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        allNotesOff (m.isAllSoundOff());
    }
}

void VoiceManager::allNotesOff (bool hard)
{
    for (auto& v : voices)
    {
        if (! v.isActive()) continue;
        if (hard) v.kill(); else v.release();
    }
    numHeld = 0;
}

void VoiceManager::render (float* outL, float* outR, float* srcL, float* srcR, int n, const RenderContext& ctx)
{
    for (int i = 0; i < maxVoices; ++i)
    {
        auto& v = voices[(size_t) i];
        if (v.isActive())
            v.render (outL, outR, srcL, srcR, n, ctx);
    }
}

} // namespace am
