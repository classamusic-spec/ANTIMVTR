#include "AMStepEditor.h"

namespace am::ui
{

AMStepEditor::AMStepEditor (juce::Colour a) : accent (a)
{
    setWantsKeyboardFocus (false);
    steps.fill (1.0f);
}

void AMStepEditor::setNumSteps (int n)
{
    n = juce::jlimit (1, kMaxSteps, n);
    if (n == numSteps) return;
    numSteps = n;
    repaint();
}

void AMStepEditor::setStep (int index, float value, juce::NotificationType notify)
{
    if (index < 0 || index >= kMaxSteps) return;
    value = bipolar ? juce::jlimit (-1.0f, 1.0f, value) : juce::jlimit (0.0f, 1.0f, value);
    if (std::abs (steps[(size_t) index] - value) < 0.0005f) return;
    steps[(size_t) index] = value;
    repaint();
    if (notify != juce::dontSendNotification && onStepChanged) onStepChanged (index, value);
}

void AMStepEditor::setSteps (const std::vector<float>& values, juce::NotificationType notify)
{
    for (int i = 0; i < kMaxSteps && i < (int) values.size(); ++i)
        steps[(size_t) i] = bipolar ? juce::jlimit (-1.0f, 1.0f, values[(size_t) i]) : juce::jlimit (0.0f, 1.0f, values[(size_t) i]);
    repaint();
    if (notify != juce::dontSendNotification && onPatternChanged) onPatternChanged();
}

std::vector<float> AMStepEditor::getSteps() const
{
    return std::vector<float> (steps.begin(), steps.begin() + numSteps);
}

void AMStepEditor::setPlayhead (float position)
{
    if (std::abs (position - playhead) < 0.02f) return;
    playhead = position;
    repaint();
}

juce::Rectangle<float> AMStepEditor::field() const
{
    auto b = getLocalBounds().toFloat();
    const float footer = juce::jlimit (10.0f, 16.0f, b.getHeight() * 0.12f);
    return b.withTrimmedBottom (footer).reduced (4.0f, 4.0f);
}

int AMStepEditor::stepAt (float x) const
{
    const auto f = field();
    return juce::jlimit (0, numSteps - 1, (int) ((x - f.getX()) / f.getWidth() * (float) numSteps));
}

float AMStepEditor::valueAt (float y) const
{
    const auto f = field();
    const float v = 1.0f - (y - f.getY()) / f.getHeight();
    return bipolar ? juce::jlimit (-1.0f, 1.0f, v * 2.0f - 1.0f) : juce::jlimit (0.0f, 1.0f, v);
}

void AMStepEditor::paintStep (const juce::MouseEvent& e)
{
    const int s = stepAt (e.position.x);
    const float v = e.mods.isShiftDown() && lastPaintedStep >= 0 ? lastPaintedValue : valueAt (e.position.y);
    // fill every step between the previous drag position and this one so fast drags draw lines
    if (lastPaintedStep >= 0 && std::abs (s - lastPaintedStep) > 1)
    {
        const int dir = s > lastPaintedStep ? 1 : -1;
        for (int i = lastPaintedStep + dir; i != s; i += dir)
        {
            const float t = (float) (i - lastPaintedStep) / (float) (s - lastPaintedStep);
            setStep (i, lastPaintedValue + (v - lastPaintedValue) * t);
        }
    }
    setStep (s, v);
    lastPaintedStep = s; lastPaintedValue = v;
    tip.showFor (*this, e.getPosition(), "STEP " + juce::String (s + 1) + "   " + juce::String (v, 2));
}

void AMStepEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { showMenu(); return; }
    dragging = true;
    lastPaintedStep = -1;
    paintStep (e);
}

void AMStepEditor::mouseDrag (const juce::MouseEvent& e) { if (dragging) paintStep (e); }
void AMStepEditor::mouseUp (const juce::MouseEvent&) { dragging = false; lastPaintedStep = -1; repaint(); }
void AMStepEditor::mouseMove (const juce::MouseEvent& e) { const int s = stepAt (e.position.x); if (s != hoverStep) { hoverStep = s; repaint(); } }

void AMStepEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    setStep (stepAt (e.position.x), bipolar ? 0.0f : 1.0f);
}

void AMStepEditor::showMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("PATTERN");
    m.addItem (1, "Reset (all on)");
    m.addItem (2, "Clear");
    m.addItem (3, "Ramp up");
    m.addItem (4, "Ramp down");
    m.addItem (5, "Alternate");
    m.addItem (6, "Random");
    m.addItem (7, "Invert");
    m.addItem (8, "Shift left");
    m.addItem (9, "Shift right");
    juce::Component::SafePointer<AMStepEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r) { if (safe != nullptr && r > 0) safe->applyPattern (r); });
}

void AMStepEditor::applyPattern (int id)
{
    const float lo = bipolar ? -1.0f : 0.0f;
    juce::Random rng;
    for (int i = 0; i < numSteps; ++i)
    {
        const float u = numSteps > 1 ? (float) i / (float) (numSteps - 1) : 0.0f;
        float v = steps[(size_t) i];
        switch (id)
        {
            case 1: v = 1.0f; break;
            case 2: v = lo; break;
            case 3: v = lo + (1.0f - lo) * u; break;
            case 4: v = 1.0f - (1.0f - lo) * u; break;
            case 5: v = (i % 2 == 0) ? 1.0f : lo; break;
            case 6: v = lo + (1.0f - lo) * rng.nextFloat(); break;
            case 7: v = bipolar ? -v : 1.0f - v; break;
            default: break;
        }
        steps[(size_t) i] = v;
    }
    if (id == 8) std::rotate (steps.begin(), steps.begin() + 1, steps.begin() + numSteps);
    if (id == 9) std::rotate (steps.begin(), steps.begin() + (numSteps - 1), steps.begin() + numSteps);
    repaint();
    if (onPatternChanged) onPatternChanged();
}

void AMStepEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto f = field();
    const float corner = juce::jmin (10.0f, b.getHeight() * 0.08f);
    draw::insetSurface (g, b.withBottom (f.getBottom() + 4.0f), corner);

    const float stepW = f.getWidth() / (float) numSteps;
    const float gap = juce::jlimit (1.0f, 4.0f, stepW * 0.12f);
    const float zeroY = bipolar ? f.getCentreY() : f.getBottom();

    // grid: beat lines every 4 steps
    for (int i = 0; i <= numSteps; ++i)
    {
        const float x = f.getX() + stepW * (float) i;
        g.setColour (juce::Colours::white.withAlpha (i % 4 == 0 ? 0.07f : 0.03f));
        g.drawLine (x, f.getY(), x, f.getBottom(), 1.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawLine (f.getX(), zeroY, f.getRight(), zeroY, 1.0f);

    // steps
    const int playStep = playhead >= 0.0f ? ((int) playhead) % juce::jmax (1, numSteps) : -1;
    for (int i = 0; i < numSteps; ++i)
    {
        const float v = steps[(size_t) i];
        const float x = f.getX() + stepW * (float) i + gap * 0.5f;
        const float top = bipolar ? zeroY - v * f.getHeight() * 0.5f : f.getBottom() - v * f.getHeight();
        auto bar = juce::Rectangle<float> (x, juce::jmin (top, zeroY), stepW - gap, std::abs (zeroY - top));
        const bool active = i == playStep;
        const bool hv = i == hoverStep;
        const float alpha = active ? 0.95f : (hv ? 0.8f : 0.6f);
        if (bar.getHeight() > 0.5f)
        {
            juce::ColourGradient grad (accent.withAlpha (alpha), 0.0f, bar.getY(), accent.withAlpha (alpha * 0.25f), 0.0f, bar.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (bar, 2.0f);
            // bright cap
            g.setColour (accent.brighter (0.4f).withAlpha (alpha));
            g.fillRect (bar.withHeight (juce::jmin (2.0f, bar.getHeight())));
            if (active) draw::glowRoundedRect (g, bar, 2.0f, accent, 8.0f, 0.7f);
        }
        else
        {
            g.setColour (accent.withAlpha (0.25f));
            g.fillRect (juce::Rectangle<float> (x, zeroY - 1.0f, stepW - gap, 2.0f));
        }
        if (hv && ! dragging)
        {
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.fillRect (juce::Rectangle<float> (f.getX() + stepW * (float) i, f.getY(), stepW, f.getHeight()));
        }
    }

    // playhead column
    if (playhead >= 0.0f)
    {
        const float x = f.getX() + stepW * std::fmod (playhead, (float) numSteps);
        juce::ColourGradient grad (Theme::textPrimary.withAlpha (0.16f), x, 0.0f, Theme::textPrimary.withAlpha (0.0f), x + stepW, 0.0f, false);
        g.setGradientFill (grad);
        g.fillRect (juce::Rectangle<float> (x, f.getY(), stepW, f.getHeight()));
        juce::Path line; line.startNewSubPath (x, f.getY()); line.lineTo (x, f.getBottom());
        draw::glowPath (g, line, Theme::textPrimary, 1.0f, 6.0f, 0.6f);
    }

    draw::screenGlass (g, b.withBottom (f.getBottom() + 4.0f), corner, 0.85f);

    // step numbers
    const float h = juce::jlimit (7.0f, 9.5f, (b.getBottom() - f.getBottom()) * 0.6f);
    for (int i = 0; i < numSteps; ++i)
    {
        if (numSteps > 16 && i % 2 == 1) continue;
        auto cell = juce::Rectangle<float> (f.getX() + stepW * (float) i, f.getBottom() + 4.0f, stepW, b.getBottom() - f.getBottom() - 4.0f);
        draw::trackedText (g, juce::String (i + 1), cell, juce::Justification::centred, Theme::valueFont (h), i == playStep ? Theme::textPrimary : Theme::textDim);
    }
}

} // namespace am::ui
