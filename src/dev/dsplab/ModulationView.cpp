#include "ModulationView.h"

#include "plugin/AntiMatrProcessor.h"

namespace am::dev
{

using namespace am::ui;

namespace
{
    juce::Colour colourForGroup (ModSourceGroup g) noexcept
    {
        switch (g)
        {
            case ModSourceGroup::LFO:      return Theme::amber;
            case ModSourceGroup::Envelope: return Theme::cyan;
            case ModSourceGroup::Chaos:    return Theme::magenta;
            case ModSourceGroup::Macro:    return Theme::ivory;
            default:                       return Theme::blue;
        }
    }

    juce::String naturalUnits (Param p, float value)
    {
        const auto& d = ParameterRegistry::get (p);
        juce::String s = (value >= 0.0f ? "+" : "") + juce::String (value, std::abs (value) < 10.0f ? 3 : 1);
        if (d.unit[0] != 0) s << " " << d.unit;
        return s;
    }
}

//==============================================================================
/** One cell per source: name, rolling scope and the current value. */
class ModulationView::SourceGrid : public juce::Component
{
public:
    static constexpr int kHistory = 256;

    void push (const ModulationSnapshot& s)
    {
        for (int i = 1; i < kNumModSources; ++i)
        {
            const float v = std::isfinite (s.sourceValue[i]) ? juce::jlimit (-1.0f, 1.0f, s.sourceValue[i]) : 0.0f;
            history[(size_t) i][(size_t) write] = v;
        }
        write = (write + 1) % kHistory;
        repaint();
    }

    void setActive (const uint8_t* used) { std::memcpy (active, used, sizeof (active)); }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const int count = kNumModSources - 1;
        const int columns = juce::jlimit (2, 7, juce::roundToInt (b.getWidth() / 150.0f));
        const int rows = (count + columns - 1) / columns;
        const float cellW = b.getWidth() / (float) columns;
        const float cellH = b.getHeight() / (float) rows;

        for (int i = 0; i < count; ++i)
        {
            const auto source = (ModSource) (i + 1);
            const auto cell = juce::Rectangle<float> (b.getX() + (float) (i % columns) * cellW,
                                                      b.getY() + (float) (i / columns) * cellH,
                                                      cellW, cellH).reduced (2.0f);
            if (cell.getWidth() < 12.0f || cell.getHeight() < 10.0f) continue;

            const bool used = active[i + 1] != 0;
            const auto colour = colourForGroup (modSourceGroup (source));
            const float alpha = used ? 1.0f : 0.35f;

            draw::insetSurface (g, cell, 3.0f);

            auto text = cell.reduced (3.0f, 1.0f);
            auto header = text.removeFromTop (juce::jmin (11.0f, text.getHeight() * 0.34f));
            const float value = history[(size_t) (i + 1)][(size_t) ((write + kHistory - 1) % kHistory)];
            plot::caption (g, juce::String (modSourceName (source)).toUpperCase(),
                           header.removeFromLeft (header.getWidth() * 0.62f), colour.withAlpha (alpha), 8.5f);
            plot::caption (g, juce::String (value, 2), header, Theme::textValue.withAlpha (alpha), 8.5f,
                           juce::Justification::centredRight);

            // Rolling scope: zero line in the middle, oldest sample on the left.
            const auto scope = text.reduced (0.0f, 1.0f);
            if (scope.getHeight() < 6.0f) continue;
            const float mid = scope.getCentreY();
            g.setColour (Theme::border);
            g.drawHorizontalLine ((int) mid, scope.getX(), scope.getRight());

            juce::Path path;
            for (int k = 0; k < kHistory; ++k)
            {
                const float v = history[(size_t) (i + 1)][(size_t) ((write + k) % kHistory)];
                const float x = scope.getX() + scope.getWidth() * (float) k / (float) (kHistory - 1);
                const float y = mid - juce::jlimit (-1.0f, 1.0f, v) * scope.getHeight() * 0.46f;
                if (k == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            g.setColour (colour.withAlpha (used ? 0.9f : 0.3f));
            g.strokePath (path, juce::PathStrokeType (1.1f));
        }
    }

private:
    std::array<std::array<float, kHistory>, kNumModSources> history {};
    uint8_t active[kNumModSources] {};
    int write = 0;
};

//==============================================================================
ModulationView::ModulationView()
{
    facts.setRowHeight (14.0f);
    facts.setLabelWidthFraction (0.58f);
    addAndMakeVisible (facts);

    grid = std::make_unique<SourceGrid>();
    sourcePanel.addAndMakeVisible (*grid);
    addAndMakeVisible (sourcePanel);

    routingTable.setColumns ({ { "SOURCE", 74, false }, { "DESTINATION", 132, false }, { "DEPTH", 50, true },
                              { "POL", 34, false }, { "SCOPE", 44, false }, { "NOW", 62, true },
                              { "MIN", 58, true }, { "MAX", 58, true } });
    routingTable.setDefaultSort (1, true);
    routingPanel.addAndMakeVisible (routingTable);
    addAndMakeVisible (routingPanel);
}

ModulationView::~ModulationView() = default;

void ModulationView::resized()
{
    auto area = getLocalBounds();
    const int gap = juce::jmax (4, area.getHeight() / 90);

    facts.setBounds (area.removeFromTop (juce::jlimit (70, 120, area.getHeight() / 6)));
    area.removeFromTop (gap);

    auto left = area.removeFromLeft (juce::roundToInt ((float) area.getWidth() * 0.46f));
    area.removeFromLeft (gap);
    sourcePanel.setBounds (left);
    grid->setBounds (sourcePanel.contentBounds());
    routingPanel.setBounds (area);
    routingTable.setBounds (routingPanel.contentBounds());
}

void ModulationView::updateFrame (const LabFrame& f)
{
    const auto& snapshot = f.diagnostics.modulationSnapshots.latest();
    const auto& routings = f.processor.getModRoutings();

    // Which sources are actually driving something?
    uint8_t used[kNumModSources] {};
    for (int i = 0; i < routings.size(); ++i)
        if (routings[i].enabled) used[(int) routings[i].source] = 1;
    grid->setActive (used);
    grid->push (snapshot);

    facts.setRows ({
        { "Routings",           juce::String (snapshot.numRoutings) + " (" + juce::String (snapshot.numEnabled) + " enabled)" },
        { "Per-voice routings", juce::String (snapshot.numPolyRoutings) },
        { "Control slice",      snapshot.controlBlock > 0
                                    ? juce::String (snapshot.controlBlock) + " smp  ("
                                      + juce::String (f.sampleRate / juce::jmax (1, snapshot.controlBlock), 0) + " Hz)"
                                    : juce::String ("idle (block rate)") },
        { "Focus voice",        snapshot.focusVoice >= 0 ? juce::String (snapshot.focusVoice) : juce::String ("-") },
        { "Modulated params",   [&snapshot]
                                {
                                    int n = 0;
                                    for (int i = 0; i < kNumParams; ++i) if (snapshot.targeted[i] > 0) ++n;
                                    return juce::String (n);
                                }() }
    });

    std::vector<LabTable::Row> rows;
    rows.reserve ((size_t) routings.size());
    for (int i = 0; i < routings.size(); ++i)
    {
        const auto& r = routings[i];
        const auto& d = ParameterRegistry::get (r.target);
        const int index = paramIndex (r.target);
        const float now = snapshot.modulation[index];
        const bool perVoice = modSourceIsPerVoice (r.source, f.processor.currentParamValues());
        const auto tint = r.enabled ? Theme::textPrimary : Theme::textDim;

        rows.push_back ({
            { modSourceName (r.source), (double) (int) r.source, colourForGroup (modSourceGroup (r.source)).withAlpha (r.enabled ? 1.0f : 0.4f) },
            { juce::String (d.id), (double) index, tint },
            { juce::String (r.depth, 3), (double) r.depth, tint },
            { r.bipolar ? "BI" : "UNI", r.bipolar ? 1.0 : 0.0, tint },
            { perVoice ? "voice" : "mono", perVoice ? 1.0 : 0.0, perVoice ? Theme::cyan : Theme::textSecondary },
            { naturalUnits (r.target, now), (double) now, tint },
            { naturalUnits (r.target, snapshot.modMin[index]), (double) snapshot.modMin[index], Theme::textSecondary },
            { naturalUnits (r.target, snapshot.modMax[index]), (double) snapshot.modMax[index], Theme::textSecondary }
        });
    }
    routingTable.setRows (std::move (rows));
}

} // namespace am::dev
