#include "DiagnosticReport.h"

namespace am::dev
{

namespace
{
    const char* qualityName (uint8_t q) noexcept
    {
        switch ((Quality) q)
        {
            case Quality::Eco:    return "ECO";
            case Quality::Normal: return "NORMAL";
            case Quality::High:   return "HIGH";
            case Quality::Ultra:  return "ULTRA";
            default:              return "UNKNOWN";
        }
    }

    const char* dryModeName (uint8_t d) noexcept
    {
        switch ((DryMode) d)
        {
            case DryMode::FullSynth:       return "FULL SYNTH";
            case DryMode::SourceOnly:      return "SOURCE ONLY";
            case DryMode::MatterOnly:      return "MATTER ONLY";
            case DryMode::MatterAndEvolve: return "MATTER + EVOLVE";
            default:                       return "UNKNOWN";
        }
    }

    const char* stageName (int i) noexcept
    {
        switch ((Stage) i)
        {
            case Stage::Source:       return "Source";
            case Stage::PostMatter:   return "PostMatter";
            case Stage::PostEvolve:   return "PostEvolve";
            case Stage::PostFracture: return "PostFracture";
            case Stage::PostSpace:    return "PostSpace";
            case Stage::Master:       return "Master";
            default:                  return "Unknown";
        }
    }

    juce::var roundedVar (double v, int decimals = 4) noexcept
    {
        if (! std::isfinite (v)) return juce::var (0.0);
        const double scale = std::pow (10.0, decimals);
        return juce::var (std::round (v * scale) / scale);
    }
}

DiagnosticReportInput DiagnosticReportInput::environment()
{
    DiagnosticReportInput in;
    in.buildVersion = ANTIMATR_VERSION_STRING;
    in.os = juce::SystemStats::getOperatingSystemName()
          + " (" + juce::String (juce::SystemStats::getNumCpus()) + " cpus, "
          + juce::String (juce::SystemStats::getMemorySizeInMegabytes()) + " MB)";
    in.timestamp = juce::Time::getCurrentTime().toISO8601 (true);
    return in;
}

int DiagnosticReport::numActiveParameters (const ParamValues& values) noexcept
{
    int active = 0;
    for (const auto& d : ParameterRegistry::all())
    {
        const float v = values[(size_t) paramIndex (d.param)];
        if (std::abs (v - d.defaultValue) > 1.0e-6f)
            ++active;
    }
    return active;
}

juce::var DiagnosticReport::toVar (const DiagnosticReportInput& in)
{
    const auto& s = in.snapshot;
    auto* root = new juce::DynamicObject();

    root->setProperty ("report", "ANTI-MATR diagnostic report");
    root->setProperty ("schema", 1);
    root->setProperty ("build", in.buildVersion.isNotEmpty() ? in.buildVersion : juce::String (ANTIMATR_VERSION_STRING));
    root->setProperty ("os", in.os);
    root->setProperty ("timestamp", in.timestamp.isNotEmpty() ? in.timestamp : juce::Time::getCurrentTime().toISO8601 (true));
    root->setProperty ("containsAudio", false);

    // ---- engine
    {
        auto* e = new juce::DynamicObject();
        e->setProperty ("sampleRate", s.sampleRate);
        e->setProperty ("blockSize", s.blockSize);
        e->setProperty ("quality", qualityName (s.quality));
        e->setProperty ("dryMode", dryModeName (s.dryMode));
        e->setProperty ("activeVoices", s.activeVoices);
        e->setProperty ("maxVoices", s.maxVoices);
        e->setProperty ("latencySamples", s.latencySamples);
        e->setProperty ("sampleTime", (juce::int64) s.sampleTime);
        e->setProperty ("fractureFFTSize", s.fractureFFTSize);
        e->setProperty ("fractureHop", s.fractureHop);
        e->setProperty ("fractureActivity", roundedVar (s.fractureActivity));
        e->setProperty ("eventsDropped", (juce::int64) s.eventsDropped);
        root->setProperty ("engine", juce::var (e));
    }

    // ---- matter
    {
        auto* m = new juce::DynamicObject();
        m->setProperty ("focusVoice", s.focusVoice);
        m->setProperty ("focusNote", s.focusNote);
        m->setProperty ("fundamentalHz", roundedVar (s.fundamentalHz, 2));
        m->setProperty ("numNodes", s.numNodes);
        m->setProperty ("activeNodes", s.activeNodes);
        m->setProperty ("clusterCount", s.clusterCount);
        m->setProperty ("numEdges", s.numEdges);
        m->setProperty ("topologySeed", (juce::int64) s.topologySeed);
        m->setProperty ("averageCoupling", roundedVar (s.averageCoupling));
        m->setProperty ("maxCoupling", roundedVar (s.maxCoupling));
        m->setProperty ("energy", roundedVar (s.matterEnergy));
        m->setProperty ("materialA", (int) s.materialA);
        m->setProperty ("materialB", (int) s.materialB);
        m->setProperty ("materialBlend", roundedVar (s.materialBlend));
        root->setProperty ("matter", juce::var (m));
    }

    // ---- stage levels (dBFS, no audio content)
    {
        auto* levels = new juce::DynamicObject();
        for (int i = 0; i < (int) Stage::Count; ++i)
        {
            auto* st = new juce::DynamicObject();
            st->setProperty ("rmsDb", roundedVar (gainToDb (s.stages[i].rms), 2));
            st->setProperty ("peakDb", roundedVar (gainToDb (s.stages[i].peak), 2));
            levels->setProperty (stageName (i), juce::var (st));
        }
        root->setProperty ("stageLevels", juce::var (levels));
    }

    // ---- cpu profile
    {
        auto* cpu = new juce::DynamicObject();
        for (int i = 0; i < PerformanceProfiler::kNumSubsystems; ++i)
        {
            auto* sub = new juce::DynamicObject();
            sub->setProperty ("avgPercent", roundedVar (s.perf.avgPercent[i], 3));
            sub->setProperty ("peakPercent", roundedVar (s.perf.peakPercent[i], 3));
            sub->setProperty ("movingPercent", roundedVar (s.perf.movingPercent[i], 3));
            cpu->setProperty (subsystemName ((Subsystem) i), juce::var (sub));
        }
        cpu->setProperty ("totalAvgPercent", roundedVar (s.perf.totalAvgPercent, 3));
        cpu->setProperty ("totalPeakPercent", roundedVar (s.perf.totalPeakPercent, 3));
        cpu->setProperty ("totalMovingPercent", roundedVar (s.perf.totalMovingPercent, 3));
        cpu->setProperty ("lastBlockMicros", roundedVar (s.perf.lastBlockMicros, 2));
        cpu->setProperty ("budgetMicros", roundedVar (s.perf.budgetMicros, 2));
        cpu->setProperty ("blocksMeasured", (juce::int64) s.perf.blocksMeasured);
        cpu->setProperty ("overruns", (juce::int64) s.perf.overruns);
        root->setProperty ("cpu", juce::var (cpu));
    }

    // ---- safety
    {
        auto* safety = new juce::DynamicObject();
        for (int i = 0; i < SafetyMonitor::kNumEvents; ++i)
        {
            auto* c = new juce::DynamicObject();
            c->setProperty ("count", (juce::int64) s.safety.counts[i]);
            if (s.safety.counts[i] > 0)
            {
                c->setProperty ("lastSubsystem", subsystemName ((Subsystem) s.safety.lastSubsystem[i]));
                c->setProperty ("lastVoice", (int) s.safety.lastVoice[i]);
            }
            safety->setProperty (SafetyMonitor::eventName ((SafetyEvent) i), juce::var (c));
        }
        safety->setProperty ("total", (juce::int64) s.safety.total);
        root->setProperty ("safety", juce::var (safety));
    }

    // ---- preset + parameters
    {
        auto* preset = new juce::DynamicObject();
        preset->setProperty ("name", in.presetName);
        preset->setProperty ("tags", in.presetTags.joinIntoString (", "));
        preset->setProperty ("abSlot", in.abSlot == 0 ? "A" : "B");
        root->setProperty ("preset", juce::var (preset));

        auto* params = new juce::DynamicObject();
        auto* changed = new juce::DynamicObject();
        for (const auto& d : ParameterRegistry::all())
        {
            const float v = in.parameters[(size_t) paramIndex (d.param)];
            params->setProperty (d.id, roundedVar (v, 5));
            if (std::abs (v - d.defaultValue) > 1.0e-6f)
                changed->setProperty (d.id, roundedVar (v, 5));
        }
        root->setProperty ("parameters", juce::var (params));
        root->setProperty ("changedParameters", juce::var (changed));
        root->setProperty ("numParameters", kNumParams);
        root->setProperty ("numChangedParameters", numActiveParameters (in.parameters));
    }

    if (! in.extras.empty())
    {
        auto* extras = new juce::DynamicObject();
        for (const auto& e : in.extras)
            extras->setProperty (e.first, roundedVar (e.second, 4));
        root->setProperty ("measurements", juce::var (extras));
    }

    return juce::var (root);
}

juce::String DiagnosticReport::toJson (const DiagnosticReportInput& in)
{
    return juce::JSON::toString (toVar (in), false);
}

juce::File DiagnosticReport::defaultFile()
{
    auto desktop = juce::File::getSpecialLocation (juce::File::userDesktopDirectory);
    if (! desktop.isDirectory())
        desktop = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    const auto stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
    return desktop.getChildFile ("antimatr-diagnostics-" + stamp + ".json");
}

juce::Result DiagnosticReport::writeTo (const juce::File& file, const DiagnosticReportInput& in)
{
    const auto parent = file.getParentDirectory();
    if (! parent.isDirectory())
    {
        const auto r = parent.createDirectory();
        if (r.failed()) return r;
    }
    if (! file.replaceWithText (toJson (in)))
        return juce::Result::fail ("Could not write " + file.getFullPathName());
    return juce::Result::ok();
}

} // namespace am::dev
