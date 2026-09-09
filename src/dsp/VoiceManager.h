#pragma once

#include "AntiMatrVoice.h"
#include <vector>

namespace am
{

struct Diagnostics;

/**
    Allocates voices, handles MIDI (note on/off, sustain, pitch bend, channel
    and polyphonic pressure, mod wheel, all-notes-off), voice stealing and
    the POLY / MONO / LEGATO modes with glide.

    MPE readiness: pitch bend and pressure are tracked per MIDI channel and
    applied to the voices playing on that channel.
*/
class VoiceManager
{
public:
    VoiceManager();

    void prepare (double sampleRate, int maxBlockSize, Diagnostics* diagnostics);
    void reset();

    void setMaxVoices (int n) noexcept;
    int  getMaxVoices() const noexcept { return maxVoices; }

    /** Handles a MIDI message at the current render position. */
    void handleMidi (const juce::MidiMessage& m, const ParamValues& params, Quality quality);

    /** Renders all active voices (accumulating) for n samples. */
    void render (float* outL, float* outR, float* srcL, float* srcR, int n, const RenderContext& ctx);

    int activeVoiceCount() const noexcept;
    int mostRecentVoice() const noexcept { return lastStartedVoice; }
    const AntiMatrVoice& voice (int i) const noexcept { return voices[(size_t) i]; }
    AntiMatrVoice& voice (int i) noexcept { return voices[(size_t) i]; }

    void allNotesOff (bool hard);

private:
    void noteOn (int channel, int midiNote, float velocity, const ParamValues& params, Quality quality);
    void noteOff (int channel, int midiNote, const ParamValues& params, Quality quality);
    int  findFreeVoice() const noexcept;
    int  stealVoice() noexcept;
    int  findVoiceForNote (int channel, int midiNote) const noexcept;
    float bendSemisFor (int channel, const ParamValues& params) const noexcept;

    std::vector<AntiMatrVoice> voices;   // heap allocated once (large per-voice buffers)
    int maxVoices = kDefaultVoices;
    Diagnostics* diag = nullptr;
    double sr = 48000.0;

    // MIDI state
    std::array<float, 17> channelBend {};      // -1..1 per channel (index 1..16, 0 = global)
    std::array<float, 17> channelPressure {};
    float modWheel = 0.0f;
    bool sustainDown = false;
    uint32_t noteCounter = 0;
    int lastStartedVoice = -1;

    // Mono / legato note stack (held keys, most recent last)
    static constexpr int kMaxHeld = 32;
    std::array<int, kMaxHeld> heldNotes {};
    std::array<float, kMaxHeld> heldVelocities {};
    int numHeld = 0;
    double lastMonoFrequency = 0.0;

    void pushHeld (int note, float vel) noexcept;
    void removeHeld (int note) noexcept;
};

} // namespace am
