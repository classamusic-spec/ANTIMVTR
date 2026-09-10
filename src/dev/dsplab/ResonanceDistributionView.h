#pragma once

#include "LabWidgets.h"
#include "dev/diagnostics/MatterAnalysis.h"

namespace am::dev
{

/**
    RESONANCE DISTRIBUTION (§73)

    Target versus actual frequencies on a log axis (bar height = energy, the
    dim bar behind it = the material's requested weight), a weighted histogram
    of frequency/fundamental ratios with the integer harmonics marked, and the
    harmonicity deviation of every node in cents from its nearest harmonic.
*/
class FrequencyDistributionView : public LabPanel
{
public:
    FrequencyDistributionView();

    void setNodes (const NodeDiag* nodes, int numNodes, float fundamentalHz);
    void paint (juce::Graphics& g) override;

private:
    const NodeDiag* nodes = nullptr;
    int numNodes = 0;
    float fundamentalHz = 0.0f;
};

//==============================================================================
class RatioHistogramView : public LabPanel
{
public:
    RatioHistogramView();

    void setNodes (const NodeDiag* nodes, int numNodes, float fundamentalHz);
    void paint (juce::Graphics& g) override;

private:
    std::vector<float> bins;
    float maxRatio = 16.0f;
    int numNodes = 0;
};

//==============================================================================
class HarmonicityView : public LabPanel
{
public:
    HarmonicityView();

    void setNodes (const NodeDiag* nodes, int numNodes, float fundamentalHz);
    void paint (juce::Graphics& g) override;

private:
    const NodeDiag* nodes = nullptr;
    int numNodes = 0;
    float fundamentalHz = 0.0f;
    MatterStats stats;
};

} // namespace am::dev
