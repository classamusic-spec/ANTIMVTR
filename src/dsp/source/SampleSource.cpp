#include "SampleSource.h"
#include "dev/diagnostics/Diagnostics.h"

namespace am
{

using excitation::softLimit;

namespace
{
    constexpr double kMinRate = 1.0 / 256.0;   ///< slowest read (a 8 octave transposition down)
    constexpr double kMaxRate = 32.0;          ///< fastest read; also bounds the anti-alias tap count
    constexpr double kGrainOverlap = 3.0;      ///< average number of grains sounding at once
}

//==============================================================================
void SampleSource::prepare (double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused (maxBlockSize);
    sr = juce::jmax (8000.0, sampleRate);
    excitation::warmTables();
    dcL.prepare (sr, 10.0f);
    dcR.prepare (sr, 10.0f);
    fadeStep = (float) (1.0 / juce::jmax (1.0, kFadeSeconds * sr));
    residueDecay = std::exp (-4.0f / (float) juce::jmax (1.0, kFadeSeconds * sr));
    reset();
}

void SampleSource::reset()
{
    pos = 0.0;
    cloudPos = 0.0;
    grainCountdown = 0.0;
    finished = false;
    gate = false;
    sample = nullptr;
    numGrains = 0;
    for (auto& g : grains) g = Grain();
    residueL = residueR = 0.0f;
    fadeGain = 1.0f;
    lastEnergy = 0.0f;
    dcL.reset();
    dcR.reset();
}

void SampleSource::noteOn (const NoteState& note, const ParamValues& params)
{
    gate = true;
    noteId = note.noteId;
    p.mode     = (Mode) juce::jlimit (0, (int) Mode::Count - 1, paramChoice (params, Param::sampleMode));
    p.start    = clamp01 (paramValue (params, Param::sampleStart));
    p.end      = clamp01 (paramValue (params, Param::sampleEnd));
    grainRng.reseed (hashSeed (noteId, 0x5A3D11u));
    updateBounds();
    restart();
}

void SampleSource::noteOff()
{
    gate = false;
}

//==============================================================================
void SampleSource::readParams (const RenderContext& ctx, const NoteState& note)
{
    p.mode       = (Mode) juce::jlimit (0, (int) Mode::Count - 1, ctx.choice (Param::sampleMode));
    p.level      = clamp01 (ctx.param (Param::sampleLevel));
    p.start      = clamp01 (ctx.param (Param::sampleStart));
    p.end        = clamp01 (ctx.param (Param::sampleEnd));
    p.grain      = clamp01 (ctx.param (Param::sampleGrain));
    p.spread     = clamp01 (ctx.param (Param::sampleSpread));
    p.pitchSemis = juce::jlimit (-24.0f, 24.0f, ctx.param (Param::samplePitch));
    p.root       = juce::jlimit (0, 127, ctx.choice (Param::sampleRoot));
    p.keytrack   = ctx.flag (Param::sampleKeytrack);

    double ratio = 1.0;
    if (p.keytrack)
    {
        const double rootHz = midiNoteToHz ((double) p.root);
        const double noteHz = std::isfinite (note.frequency) ? note.frequency : (double) kMinFrequencyHz;
        ratio = juce::jlimit (1.0 / 512.0, 512.0, noteHz / juce::jmax (1.0e-3, rootHz));
    }
    ratio *= std::pow (2.0, (double) p.pitchSemis / 12.0);

    const double srRatio = sample != nullptr && sample->sampleRate > 0.0 ? sample->sampleRate / sr : 1.0;
    rate = juce::jlimit (kMinRate, kMaxRate, ratio * srRatio);

    // Averaging a few evenly spaced reads across the step is a cheap decimation
    // filter: it keeps upward transposition from folding the top octaves down.
    interpTaps = juce::jlimit (1, 4, (int) std::ceil (rate));
}

void SampleSource::updateBounds()
{
    if (sample == nullptr || sample->isEmpty())
    {
        startFrame = endFrame = crossfade = 0.0;
        return;
    }

    const double frames = (double) sample->numFrames;
    double a = (double) juce::jmin (p.start, p.end) * frames;
    double b = (double) juce::jmax (p.start, p.end) * frames;
    startFrame = juce::jlimit (0.0, frames, a);
    endFrame   = juce::jlimit (0.0, frames, b);

    const double length = endFrame - startFrame;
    // Loop crossfade: a quarter of the loop, capped at 25 ms of source material.
    crossfade = juce::jlimit (0.0, length * 0.25, 0.025 * sample->sampleRate);
}

void SampleSource::restart()
{
    finished = false;
    numGrains = 0;
    for (auto& g : grains) g.active = false;
    grainCountdown = 0.0;

    if (p.mode == Mode::Reverse)
        pos = juce::jmax (startFrame, endFrame - 1.0);
    else
        pos = startFrame;

    cloudPos = startFrame;
    fadeGain = 0.0f;    // fade the new material in; the residue covers the join
    residueL = residueR = 0.0f;
}

//==============================================================================
inline float SampleSource::readAt (int channel, double position, double readRate) const noexcept
{
    if (interpTaps <= 1) return sample->read (channel, position);

    const double step = readRate / (double) interpTaps;
    float sum = 0.0f;
    for (int k = 0; k < interpTaps; ++k)
        sum += sample->read (channel, position + ((double) k + 0.5) * step - readRate * 0.5);
    return sum / (float) interpTaps;
}

inline void SampleSource::readStereo (double position, double readRate, float& outL, float& outR) const noexcept
{
    outL = readAt (0, position, readRate);
    outR = sample->numChannels > 1 ? readAt (1, position, readRate) : outL;
}

//==============================================================================
void SampleSource::renderLinear (float* l, float* r, int n)
{
    const double length = endFrame - startFrame;
    const bool reverse = p.mode == Mode::Reverse;
    const bool loop = p.mode == Mode::Loop;
    const double step = reverse ? -rate : rate;

    if (length <= 4.0)   // degenerate selection: nothing to play
    {
        juce::FloatVectorOperations::clear (l, n);
        juce::FloatVectorOperations::clear (r, n);
        finished = ! loop;
        return;
    }

    // Follow live start/end edits without ever leaving the selection.
    if (pos < startFrame || pos > endFrame) pos = juce::jlimit (startFrame, endFrame, pos);

    const double loopEnd = endFrame - crossfade;
    const double period  = juce::jmax (1.0, length - crossfade);
    const double tailFade = juce::jmax (1.0, kFadeSeconds * sr);   // one-shot / reverse end ramp

    for (int i = 0; i < n; ++i)
    {
        float a = 0.0f, b = 0.0f;

        if (! finished)
        {
            if (loop && crossfade > 1.0 && pos > loopEnd)
            {
                const float t = (float) juce::jlimit (0.0, 1.0, (pos - loopEnd) / crossfade);
                const float w2 = excitation::Tables::sineAt (t * 0.25f);        // sin (t * pi/2)
                const float w1 = excitation::Tables::sineAt (0.25f - t * 0.25f); // cos (t * pi/2)
                float aOut, bOut, aIn, bIn;
                readStereo (pos, rate, aOut, bOut);
                readStereo (startFrame + (pos - loopEnd), rate, aIn, bIn);
                a = aOut * w1 + aIn * w2;
                b = bOut * w1 + bIn * w2;
            }
            else
            {
                readStereo (pos, rate, a, b);
            }

            if (! loop)
            {
                // Ramp the last few milliseconds down: the selection rarely ends at a zero crossing.
                const double remaining = (reverse ? pos - startFrame : endFrame - pos) / juce::jmax (kMinRate, rate);
                if (remaining < tailFade)
                {
                    const float w = (float) juce::jlimit (0.0, 1.0, remaining / tailFade);
                    a *= w;
                    b *= w;
                }
            }

            pos += step;

            if (loop)
            {
                if (pos >= endFrame) pos -= period;
                if (pos < startFrame) pos = startFrame;
            }
            else if ((! reverse && pos >= endFrame) || (reverse && pos <= startFrame))
            {
                finished = true;
            }
        }

        l[i] = a;
        r[i] = b;
    }
}

void SampleSource::spawnGrain()
{
    if (numGrains >= kMaxGrains) return;

    int slot = -1;
    for (int i = 0; i < kMaxGrains; ++i)
        if (! grains[(size_t) i].active) { slot = i; break; }
    if (slot < 0) return;

    Grain& g = grains[(size_t) slot];
    const double length = juce::jmax (1.0, endFrame - startFrame);
    const double grainSeconds = (double) excitation::expMap (p.grain, 0.005f, 0.5f);

    g.length = juce::jlimit (16, (int) (0.75 * sr), (int) (grainSeconds * sr));
    g.invLength = 1.0f / (float) g.length;
    g.age = 0;
    g.rate = rate;

    // Scatter around the cloud centre; never outside the selection.
    const double scatter = (double) p.spread * (double) grainRng.nextBipolar() * juce::jmin (length * 0.5, 0.5 * sample->sampleRate);
    double start = cloudPos + scatter;
    const double span = juce::jmax (1.0, length - (double) g.length * rate);
    if (start < startFrame) start += span;
    if (start > startFrame + span) start -= span;
    g.position = juce::jlimit (startFrame, juce::jmax (startFrame, endFrame - 1.0), start);

    // Stereo scatter grows with spread; the cloud stays centred on average.
    const float pan = (float) p.spread * grainRng.nextBipolar() * 0.8f;
    excitation::panGains (pan, g.gainL, g.gainR);
    g.active = true;
    ++numGrains;
}

void SampleSource::renderGranular (float* l, float* r, int n)
{
    juce::FloatVectorOperations::clear (l, n);
    juce::FloatVectorOperations::clear (r, n);

    const double length = endFrame - startFrame;
    if (length <= 4.0) return;

    const double naturalRate = sample->sampleRate / sr;   // cloud advances in real time (pitch independent)
    const double grainSeconds = (double) excitation::expMap (p.grain, 0.005f, 0.5f);
    const double interval = juce::jmax (16.0, grainSeconds * sr / kGrainOverlap);
    grainGain = 1.0f / std::sqrt ((float) kGrainOverlap * 0.375f);   // Hann power, uncorrelated grains

    for (int i = 0; i < n; ++i)
    {
        if (--grainCountdown <= 0.0)
        {
            spawnGrain();
            grainCountdown = interval;
        }

        float sumL = 0.0f, sumR = 0.0f;
        for (int gi = 0; gi < kMaxGrains; ++gi)
        {
            Grain& g = grains[(size_t) gi];
            if (! g.active) continue;

            const float w = excitation::Tables::hannAt ((float) g.age * g.invLength);
            float a, b;
            readStereo (g.position, g.rate, a, b);
            sumL += a * w * g.gainL;
            sumR += b * w * g.gainR;

            g.position += g.rate;
            if (++g.age >= g.length) { g.active = false; --numGrains; }
        }

        l[i] = sumL * grainGain;
        r[i] = sumR * grainGain;

        cloudPos += naturalRate;
        if (cloudPos >= endFrame) cloudPos = startFrame + std::fmod (cloudPos - startFrame, juce::jmax (1.0, length));
        if (cloudPos < startFrame) cloudPos = startFrame;
    }
}

//==============================================================================
void SampleSource::finalise (float* l, float* r, int n, const RenderContext& ctx)
{
    const float g = p.level;
    float peak = 0.0f;
    int bad = 0;

    for (int i = 0; i < n; ++i)
    {
        float a = dcL.process (l[i] * g);
        float b = dcR.process (r[i] * g);

        // Fade the (possibly brand new) material in and let the previous output
        // decay away underneath it: a sample swap can never produce a step.
        if (fadeGain < 1.0f)
        {
            a *= fadeGain;
            b *= fadeGain;
            fadeGain = juce::jmin (1.0f, fadeGain + fadeStep);
        }
        if (residueL != 0.0f || residueR != 0.0f)
        {
            a += residueL;
            b += residueR;
            residueL *= residueDecay;
            residueR *= residueDecay;
            if (std::abs (residueL) < 1.0e-6f) residueL = 0.0f;
            if (std::abs (residueR) < 1.0e-6f) residueR = 0.0f;
        }

        if (! std::isfinite (a) || ! std::isfinite (b)) { a = 0.0f; b = 0.0f; ++bad; }
        a = softLimit (a);
        b = softLimit (b);
        l[i] = a;
        r[i] = b;
        lastOutL = a;
        lastOutR = b;
        peak = juce::jmax (peak, std::abs (a), std::abs (b));
    }

    if (bad > 0)
    {
        dcL.reset();
        dcR.reset();
        residueL = residueR = 0.0f;
        pos = juce::jlimit (startFrame, endFrame, pos);
        for (auto& gr : grains) gr.active = false;
        numGrains = 0;
        if (ctx.diagnostics != nullptr)
            ctx.diagnostics->safety.note (SafetyEvent::NaN, Subsystem::Source, -1, bad);
    }

    lastEnergy = peak;
}

void SampleSource::render (float* l, float* r, int n, const RenderContext& ctx, const NoteState& note)
{
    if (n <= 0) return;

    // Pick up the sample the engine published for this block. Swapping mid-note
    // keeps the last emitted value as a decaying residue so the join is smooth.
    const SampleData* incoming = ctx.sample;
    if (incoming != sample)
    {
        const bool hadAudio = sample != nullptr && ! sample->isEmpty();
        sample = incoming;
        readParams (ctx, note);
        updateBounds();
        if (sample != nullptr && ! sample->isEmpty())
        {
            const double frames = (double) sample->numFrames;
            pos = juce::jlimit (startFrame, endFrame, juce::jlimit (0.0, frames, pos));
            cloudPos = juce::jlimit (startFrame, endFrame, cloudPos);
            if (! hadAudio || finished)
            {
                restart();
            }
            else
            {
                fadeGain = 0.0f;
                numGrains = 0;
                for (auto& g : grains) g.active = false;
            }
            // Whatever was sounding keeps decaying under the new material.
            residueL = lastOutL;
            residueR = lastOutR;
        }
    }

    if (sample == nullptr || sample->isEmpty())
    {
        juce::FloatVectorOperations::clear (l, n);
        juce::FloatVectorOperations::clear (r, n);
        lastEnergy = 0.0f;
        return;
    }

    readParams (ctx, note);
    updateBounds();

    if (p.mode == Mode::Granular) renderGranular (l, r, n);
    else                          renderLinear (l, r, n);

    finalise (l, r, n, ctx);
}

//==============================================================================
bool SampleSource::isActive() const noexcept
{
    if (p.mode == Mode::Loop || p.mode == Mode::Granular) return true;   // continuous: the amp envelope ends the voice
    return ! finished || std::abs (residueL) > 1.0e-5f || std::abs (residueR) > 1.0e-5f;
}

} // namespace am
