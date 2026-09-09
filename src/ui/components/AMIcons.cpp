#include "AMIcons.h"

namespace am::ui
{

namespace
{
    juce::Path sineWave (float cycles, float amplitude, float yCentre = 0.5f)
    {
        juce::Path p;
        const int n = 48;
        for (int i = 0; i <= n; ++i)
        {
            const float x = (float) i / (float) n;
            const float y = yCentre - amplitude * std::sin (x * cycles * juce::MathConstants<float>::twoPi);
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        return p;
    }

    void addDots (juce::Path& p, std::initializer_list<std::pair<float, float>> pts, float r)
    {
        for (auto& pt : pts) p.addEllipse (pt.first - r, pt.second - r, r * 2.0f, r * 2.0f);
    }
}

juce::Path Icons::path (Icon icon)
{
    juce::Path p;
    switch (icon)
    {
        case Icon::Wave:
            p = sineWave (1.5f, 0.28f);
            break;

        case Icon::Dust:
            addDots (p, { {0.2f,0.3f},{0.5f,0.15f},{0.8f,0.35f},{0.35f,0.55f},{0.65f,0.6f},{0.2f,0.8f},{0.55f,0.85f},{0.85f,0.75f},{0.5f,0.4f} }, 0.045f);
            break;

        case Icon::Impact:
        {
            // spiky burst
            const int spikes = 8;
            for (int i = 0; i < spikes; ++i)
            {
                const float a = (float) i / spikes * juce::MathConstants<float>::twoPi;
                const float a2 = a + juce::MathConstants<float>::pi / spikes;
                const float r1 = 0.45f, r2 = 0.18f;
                juce::Point<float> o (0.5f + r1 * std::cos (a), 0.5f + r1 * std::sin (a));
                juce::Point<float> in (0.5f + r2 * std::cos (a2), 0.5f + r2 * std::sin (a2));
                if (i == 0) p.startNewSubPath (o); else p.lineTo (o);
                p.lineTo (in);
            }
            p.closeSubPath();
            break;
        }

        case Icon::Sample:
            // bars like a sample waveform
            for (int i = 0; i < 7; ++i)
            {
                const float x = 0.1f + (float) i * 0.13f;
                const float h = 0.15f + 0.35f * std::abs (std::sin ((float) i * 1.7f + 0.4f));
                p.addRectangle (x, 0.5f - h * 0.5f, 0.06f, h);
            }
            break;

        case Icon::Gesture:
            p.startNewSubPath (0.1f, 0.7f);
            p.cubicTo (0.3f, 0.1f, 0.6f, 0.9f, 0.9f, 0.3f);
            break;

        case Icon::Bend:
            p.startNewSubPath (0.05f, 0.55f);
            p.cubicTo (0.25f, 0.55f, 0.3f, 0.1f, 0.45f, 0.1f);
            p.cubicTo (0.6f, 0.1f, 0.6f, 0.75f, 0.75f, 0.75f);
            p.cubicTo (0.85f, 0.75f, 0.9f, 0.55f, 0.95f, 0.55f);
            break;

        case Icon::Melt:
            for (int i = 0; i < 3; ++i)
            {
                const float x = 0.3f + (float) i * 0.2f;
                p.startNewSubPath (x, 0.1f);
                p.cubicTo (x - 0.08f, 0.35f, x + 0.08f, 0.55f, x, 0.9f);
            }
            break;

        case Icon::Tear:
            p.startNewSubPath (0.5f, 0.08f);
            p.lineTo (0.42f, 0.35f); p.lineTo (0.56f, 0.5f); p.lineTo (0.44f, 0.68f); p.lineTo (0.5f, 0.92f);
            p.startNewSubPath (0.28f, 0.2f); p.lineTo (0.34f, 0.8f);
            p.startNewSubPath (0.72f, 0.2f); p.lineTo (0.66f, 0.8f);
            break;

        case Icon::Magnet:
            p.addEllipse (0.3f, 0.3f, 0.4f, 0.4f);
            p.addEllipse (0.08f, 0.08f, 0.84f, 0.84f);
            p.startNewSubPath (0.12f, 0.75f); p.lineTo (0.88f, 0.25f);
            break;

        case Icon::Gravity:
            p.addEllipse (0.35f, 0.35f, 0.3f, 0.3f);
            p.addCentredArc (0.5f, 0.5f, 0.42f, 0.2f, 0.6f, 0.0f, juce::MathConstants<float>::twoPi, true);
            break;

        case Icon::Scatter:
            addDots (p, { {0.2f,0.2f},{0.75f,0.3f},{0.4f,0.6f},{0.8f,0.8f},{0.25f,0.85f} }, 0.06f);
            break;

        case Icon::Freeze:
            for (int i = 0; i < 3; ++i)
            {
                const float a = (float) i * juce::MathConstants<float>::pi / 3.0f;
                p.startNewSubPath (0.5f + 0.42f * std::cos (a), 0.5f + 0.42f * std::sin (a));
                p.lineTo (0.5f - 0.42f * std::cos (a), 0.5f - 0.42f * std::sin (a));
            }
            break;

        case Icon::Crush:
            p.addRectangle (0.15f, 0.42f, 0.7f, 0.16f);
            p.startNewSubPath (0.3f, 0.1f); p.lineTo (0.5f, 0.35f); p.lineTo (0.7f, 0.1f);
            p.startNewSubPath (0.3f, 0.9f); p.lineTo (0.5f, 0.65f); p.lineTo (0.7f, 0.9f);
            break;

        case Icon::Main:
            p.addEllipse (0.15f, 0.15f, 0.7f, 0.7f);
            p.addEllipse (0.38f, 0.38f, 0.24f, 0.24f);
            break;

        case Icon::Source:
            p = sineWave (2.0f, 0.2f);
            p.addRectangle (0.48f, 0.1f, 0.04f, 0.8f);
            break;

        case Icon::Shape:
        {
            for (int i = 0; i < 6; ++i)
            {
                const float a = (float) i / 6.0f * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi;
                juce::Point<float> pt (0.5f + 0.42f * std::cos (a), 0.5f + 0.42f * std::sin (a));
                if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            p.closeSubPath();
            break;
        }

        case Icon::Evolve:
            p.addEllipse (0.2f, 0.2f, 0.3f, 0.3f);
            p.addEllipse (0.5f, 0.2f, 0.3f, 0.3f);
            p.addEllipse (0.35f, 0.5f, 0.3f, 0.3f);
            break;

        case Icon::Fracture:
            for (int i = 0; i < 5; ++i)
            {
                const float x = 0.15f + (float) i * 0.175f;
                const float h = 0.25f + 0.55f * std::abs (std::sin ((float) i * 2.1f + 0.5f));
                p.startNewSubPath (x, 0.5f - h * 0.5f); p.lineTo (x, 0.5f + h * 0.5f);
            }
            break;

        case Icon::Space:
            p.addEllipse (0.28f, 0.28f, 0.44f, 0.44f);
            p.addCentredArc (0.5f, 0.5f, 0.48f, 0.16f, -0.5f, 0.0f, juce::MathConstants<float>::twoPi, true);
            break;

        case Icon::Mod:
            p = sineWave (1.0f, 0.3f);
            break;

        case Icon::Lab:
            p.startNewSubPath (0.35f, 0.1f); p.lineTo (0.35f, 0.45f); p.lineTo (0.15f, 0.85f); p.lineTo (0.85f, 0.85f); p.lineTo (0.65f, 0.45f); p.lineTo (0.65f, 0.1f);
            p.startNewSubPath (0.28f, 0.1f); p.lineTo (0.72f, 0.1f);
            break;

        case Icon::Settings:
        {
            const int teeth = 8;
            for (int i = 0; i < teeth * 2; ++i)
            {
                const float a = (float) i / (teeth * 2) * juce::MathConstants<float>::twoPi;
                const float r = (i % 2 == 0) ? 0.45f : 0.34f;
                juce::Point<float> pt (0.5f + r * std::cos (a), 0.5f + r * std::sin (a));
                if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            p.closeSubPath();
            p.addEllipse (0.36f, 0.36f, 0.28f, 0.28f);
            break;
        }

        case Icon::ChevronLeft:
            p.startNewSubPath (0.62f, 0.2f); p.lineTo (0.38f, 0.5f); p.lineTo (0.62f, 0.8f);
            break;

        case Icon::ChevronRight:
            p.startNewSubPath (0.38f, 0.2f); p.lineTo (0.62f, 0.5f); p.lineTo (0.38f, 0.8f);
            break;

        case Icon::Sparkle:
            p.startNewSubPath (0.5f, 0.05f); p.quadraticTo (0.5f, 0.5f, 0.95f, 0.5f);
            p.quadraticTo (0.5f, 0.5f, 0.5f, 0.95f); p.quadraticTo (0.5f, 0.5f, 0.05f, 0.5f);
            p.quadraticTo (0.5f, 0.5f, 0.5f, 0.05f); p.closeSubPath();
            break;

        case Icon::Copy:
            p.addRectangle (0.15f, 0.15f, 0.5f, 0.5f);
            p.addRectangle (0.35f, 0.35f, 0.5f, 0.5f);
            break;
    }
    return p;
}

void Icons::draw (juce::Graphics& g, Icon icon, juce::Rectangle<float> bounds, juce::Colour colour, float strokeScale)
{
    auto p = path (icon);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto square = bounds.withSizeKeepingCentre (size, size);
    p.applyTransform (juce::AffineTransform::scale (size, size).translated (square.getX(), square.getY()));

    const bool filled = (icon == Icon::Dust || icon == Icon::Sample || icon == Icon::Scatter || icon == Icon::Crush
                         || icon == Icon::Sparkle);
    g.setColour (colour);
    if (filled)
        g.fillPath (p);
    else
        g.strokePath (p, juce::PathStrokeType (juce::jmax (1.0f, size * 0.07f * strokeScale), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

} // namespace am::ui
