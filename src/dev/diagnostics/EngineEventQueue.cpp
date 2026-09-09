#include "EngineEventQueue.h"
#include "SafetyMonitor.h"

namespace am
{

const char* EngineEventQueue::typeName (EngineEventType t) noexcept
{
    switch (t)
    {
        case EngineEventType::EnginePrepared:         return "Engine prepared";
        case EngineEventType::EngineReset:            return "Engine reset";
        case EngineEventType::VoiceStarted:           return "Voice started";
        case EngineEventType::VoiceEnded:             return "Voice ended";
        case EngineEventType::VoiceStolen:            return "Voice stolen";
        case EngineEventType::PolyphonyChanged:       return "Polyphony changed";
        case EngineEventType::QualityChanged:         return "Quality changed";
        case EngineEventType::TopologyRegenerated:    return "Topology regenerated";
        case EngineEventType::MaterialChanged:        return "Material changed";
        case EngineEventType::FreezeCaptured:         return "Freeze captured";
        case EngineEventType::FFTResized:             return "FFT resized";
        case EngineEventType::SampleAnalysisComplete: return "Sample analysis complete";
        case EngineEventType::SafetyEvent:            return "Safety";
        case EngineEventType::SafetyReset:            return "Safety counters reset";
        case EngineEventType::PresetLoaded:           return "Preset loaded";
        case EngineEventType::LatencyChanged:         return "Latency changed";
        case EngineEventType::Custom:                 return "Event";
        default:                                      return "Unknown";
    }
}

juce::String EngineEventQueue::describe (const EngineEvent& e)
{
    juce::String s;
    s << "[" << juce::String (e.sampleTime) << "] " << typeName (e.type);

    if (e.type == EngineEventType::SafetyEvent)
        s << ": " << SafetyMonitor::eventName ((SafetyEvent) e.a) << " x" << (int) e.f;
    else if (e.type == EngineEventType::VoiceStarted || e.type == EngineEventType::VoiceEnded || e.type == EngineEventType::VoiceStolen)
        s << " note " << (int) e.a;
    else if (e.type == EngineEventType::PolyphonyChanged || e.type == EngineEventType::QualityChanged
             || e.type == EngineEventType::FFTResized || e.type == EngineEventType::LatencyChanged)
        s << " -> " << (int) e.a;
    else if (e.a != 0 || e.f != 0.0f)
        s << " (" << (int) e.a << ", " << juce::String (e.f, 3) << ")";

    s << " [" << subsystemName ((Subsystem) e.subsystem);
    if (e.voice >= 0) s << " v" << (int) e.voice;
    s << "]";
    return s;
}

} // namespace am
