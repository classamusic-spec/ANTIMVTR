#include "AntiMatrProcessor.h"

// The plugin entry point required by the JUCE plugin client wrappers.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new am::AntiMatrProcessor();
}
