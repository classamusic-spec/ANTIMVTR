#include "AMKnob.h"

namespace am::ui
{


AMKnob::AMKnob (const juce::String& labelText, juce::Colour accentColour)
    : label (labelText), labelUpper (labelText.toUpperCase()), accent (accentColour)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setRotaryParameters (knobart::kStartAngle, knobart::kEndAngle, true);
    setMouseDragSensitivity (240);
    setVelocityBasedMode (false);
    setDoubleClickReturnValue (true, 0.0);
    setScrollWheelEnabled (true);
    setWantsKeyboardFocus (false);
    setRepaintsOnMouseActivity (false);
    ring.setAngles (getRotaryParameters().startAngleRadians, getRotaryParameters().endAngleRadians);
    ring.setAccent (Theme::amber);
    addAndMakeVisible (ring);
    sheen.snap (proportion());
}

AMKnob::~AMKnob()
{
    if (modTarget.has_value()) ModAssign::get().removeChangeListener (this);
}

void AMKnob::setModTarget (std::optional<Param> p)
{
    if (p.has_value() == modTarget.has_value() && (! p.has_value() || *p == *modTarget)) return;
    if (modTarget.has_value() && ! p.has_value()) ModAssign::get().removeChangeListener (this);
    if (! modTarget.has_value() && p.has_value()) ModAssign::get().addChangeListener (this);
    modTarget = p;
    repaint();
}

bool AMKnob::isAssignTarget() const noexcept
{
    return modTarget.has_value() && ModAssign::get().isArmedFor (this);
}

bool AMKnob::refreshModRing (const ModulationSnapshot& snapshot)
{
    if (! modTarget.has_value()) return false;
    const int index = paramIndex (*modTarget);
    if (index < 0 || index >= kNumParams || snapshot.targeted[index] == 0)
    {
        ring.clear();
        modHasRange = false;
        return false;
    }

    const auto& d = ParameterRegistry::get (*modTarget);
    const float base = d.clampValue ((float) getValue());
    const float lo = d.toNormalised (d.clampValue (base + snapshot.modMin[index]));
    const float hi = d.toNormalised (d.clampValue (base + snapshot.modMax[index]));
    ring.setBase (d.toNormalised (base));
    ring.setCurrent (d.toNormalised (d.clampValue (base + snapshot.modulation[index])));
    ring.setRange (lo, hi);
    ring.setSourceCount ((int) snapshot.targeted[index]);

    // AMModRing keeps the range to itself, so the knob holds on to it for the orbit.
    modLo = juce::jmin (lo, hi);
    modHi = juce::jmax (lo, hi);
    modHasRange = (modHi - modLo) > 0.002f;
    return true;
}

void AMKnob::setLabel (const juce::String& text) { label = text; labelUpper = text.toUpperCase(); repaint(); }
void AMKnob::setAccent (juce::Colour c) { accent = c; repaint(); }

void AMKnob::setStyle (Style s)
{
    if (s == style) return;
    style = s;
    flareDot = -1;
    repaint();
}

void AMKnob::setHero (bool h)
{
    if (h == hero) return;
    hero = h;
    flareDot = -1;
    resized();
    repaint();
}

AMKnob::Style AMKnob::effectiveStyle() const noexcept
{
    if (style != Style::Auto) return style;
    // Too small to hold a readable ring of dots and a turned cap: go quiet.
    if (knobBounds().getWidth() < knobart::kCappedMinDiameter) return Style::Plain;
    // The subject of its panel gets the lit ring; the cluster around it keeps the
    // same hardware with the ring dark, so a page has one thing to look at.
    return hero ? Style::CappedLit : Style::CappedDark;
}

float AMKnob::proportion() const
{
    const auto range = getRange();
    return range.getLength() > 0.0 ? (float) ((getValue() - range.getStart()) / range.getLength()) : 0.0f;
}

void AMKnob::litRun (const knobart::Geometry& geo, float p, int& head, int& lo, int& hi) const
{
    head = geo.dotIndexFor (p);
    if (bipolar)
    {
        const int mid = (geo.dots - 1) / 2;
        lo = juce::jmin (mid, head);
        hi = juce::jmax (mid, head);
    }
    else
    {
        lo = 0;
        hi = head;
    }
}

void AMKnob::valueChanged()
{
    const float p = proportion();
    ring.setBase (p);

    // The cap's sheen eases behind the value instead of tracking it exactly, so the
    // metal swings into its new light rather than being repainted there.
    anim.animate (sheen, p);

    const auto geo = geometry();
    if (geo.dots > 1)
    {
        int head = 0, lo = 0, hi = 0;
        litRun (geo, p, head, lo, hi);
        if (head != flareDot)
        {
            // A dot has just changed state: flare it and let it settle.
            if (flareDot >= 0) { flare.value = 1.0f; flare.target = 0.0f; anim.kick(); }
            flareDot = head;
        }
    }
    repaint();
}

float AMKnob::labelHeight() const
{
    return layout::knobFootprint ((float) getWidth(), (float) getHeight(), hero, labelUpper.isNotEmpty()).labelHeight;
}

juce::Rectangle<float> AMKnob::knobBounds() const
{
    const auto b = getLocalBounds().toFloat();
    const auto f = layout::knobFootprint (b.getWidth(), b.getHeight(), hero, labelUpper.isNotEmpty());
    return { b.getCentreX() - f.diameter * 0.5f, b.getY() + f.top, f.diameter, f.diameter };
}

knobart::Geometry AMKnob::geometry() const
{
    return knobart::geometry (knobBounds(), effectiveStyle(),
                              getRotaryParameters().startAngleRadians, getRotaryParameters().endAngleRadians);
}

knobart::ModView AMKnob::modView() const
{
    knobart::ModView m;
    m.active = ring.isActive();
    m.base = ring.getBase();
    m.current = ring.getCurrent();
    m.sources = ring.getSourceCount();
    m.hasRange = modHasRange;
    m.lo = modLo;
    m.hi = modHi;
    return m;
}

void AMKnob::resized()
{
    // The orbit shares the knob's own coordinates, so the two never disagree about
    // where the outermost ring sits by half a pixel.
    ring.setBounds (getLocalBounds());
    juce::Slider::resized();
}

//==============================================================================
void AMKnob::ModOrbit::paint (juce::Graphics& g)
{
    const auto m = knob.modView();
    if (! m.active) return;
    const auto geo = knob.geometry();
    if (geo.diameter < 8.0f) return;
    // Amber whatever the knob's section colour is: a modulated control is
    // unmistakable because the warm ring only ever means modulation (SPEC section 8).
    knobart::modulationOrbit (g, geo, m, Theme::amber, knob.getActivity());
    knobart::sourceBadge (g, geo, m.sources, Theme::amber);
}

//==============================================================================
void AMKnob::paint (juce::Graphics& g)
{
    const auto kb = knobBounds();
    const float d = kb.getWidth();
    if (d < 4.0f) return;

    const auto dress = effectiveStyle();
    const auto geo = geometry();
    const float p = proportion();
    const float angle = geo.angleAt (p);

    // Hover is checked against where the pointer actually is, not against the eased
    // value alone: a knob that loses its mouse-exit (a page hidden under the pointer,
    // a panel that rebuilt its controls) would otherwise keep showing its value for
    // ever, and the label would never come back. Correcting it retargets the ease
    // rather than clearing it, so even the repair is a fade and not a jump.
    const bool pointerOn = dragging || (isShowing() && isMouseOverOrDragging (true));
    if (! pointerOn && hover.target != 0.0f) anim.animate (hover, 0.0f);
    const float reach = hover.value;
    const float held = press.value;
    const float lit = juce::jlimit (0.0f, 1.0f, juce::jmax (reach, held));
    const float textLit = lit;
    const float flareAmount = flare.value * flare.value;   // squared: a flare leaves quickly

    // 1. The LED ring. A dot is a hole in the panel whether or not it is lit, so
    //    every bore is drawn first and the lamp is raised into it.
    if (geo.dots > 0)
    {
        int head = 0, lo = 0, hi = 0;
        litRun (geo, p, head, lo, hi);

        // A dark ring is not a dead ring: it warms when the control is touched, or
        // when something is actually moving it.
        const float ringFade = dress == Style::CappedLit
                                 ? 1.0f
                                 : juce::jlimit (0.0f, 1.0f, 0.70f * lit + 0.55f * activity);
        const float bloom = juce::jlimit (0.0f, 1.0f, 0.30f + 0.45f * lit + 0.40f * activity);
        const float span = (float) juce::jmax (1, geo.dots / 3);

        for (int i = 0; i < geo.dots; ++i)
        {
            const auto c = geo.dotCentre (i);
            knobart::unlitDot (g, c, geo.dotRadius);
            if (i < lo || i > hi || ringFade <= 0.01f) continue;

            // The run burns hottest at its head, so the eye lands on the value; and the
            // dot that has just lit surges — brighter, fractionally larger, spilling
            // further — then settles back into the run.
            const float toHead = 1.0f - juce::jmin (1.0f, (float) std::abs (i - head) / span);
            const float surge = (i == flareDot) ? flareAmount : 0.0f;
            const float heat = 0.78f + 0.30f * toHead * toHead + 0.24f * lit + 0.18f * activity + 0.85f * surge;
            knobart::litDot (g, c, geo.dotRadius * (1.0f + 0.20f * surge), Theme::amber,
                             heat, bloom + 0.80f * surge, ringFade);
        }
    }

    // 2. Focus: while the knob is held, a thin ring of its section colour sits just
    //    outside the body. It is the one place a section accent touches a knob.
    if (held > 0.01f)
    {
        const float fr = geo.bodyRadius() + juce::jmax (1.4f, d * 0.018f);
        knobart::softLight (g, geo.centre, fr * 1.16f, accent, 0.10f * held);
        g.setColour (accent.withAlpha (0.36f * held));
        g.drawEllipse (geo.centre.x - fr, geo.centre.y - fr, fr * 2.0f, fr * 2.0f, juce::jmax (1.0f, d * 0.011f));
    }

    // 3. The body, and the cap inset into it.
    if (dress == Style::Plain)
    {
        knobart::plainDome (g, geo.body, lit, held);
        knobart::notch (g, geo, angle, lit);
    }
    else
    {
        knobart::mouldedBody (g, geo.body, lit, held);

        // The brush is machined into the cap, so it turns with the knob exactly; the
        // sheen it throws eases behind, which is what makes the metal look as though
        // it were catching the light rather than being repainted.
        const float turn = angle - geo.angleAt (0.5f);
        const float sheenTurn = geo.angleAt (sheen.value) - geo.angleAt (0.5f);
        knobart::turnedCap (g, geo.cap, turn, knobart::kLightAngle + sheenTurn, lit);

        // 4. The indicator: one cut from the top of the cap out across the body.
        knobart::indicatorCut (g, geo.centre, angle, geo.capRadius() * 0.58f, geo.bodyRadius() * 0.94f,
                               juce::jmax (1.0f, d * 0.021f), lit);
    }

    // 5. Assign mode: every modulatable knob offers itself as a destination.
    if (isAssignTarget())
    {
        const float hr = geo.modRadius;
        const auto halo = juce::Rectangle<float> (geo.centre.x - hr, geo.centre.y - hr, hr * 2.0f, hr * 2.0f);
        knobart::softLight (g, geo.centre, hr * 1.25f, Theme::amber, 0.20f + 0.14f * lit);
        g.setColour (Theme::amber.withAlpha (0.60f + 0.40f * lit));
        g.drawEllipse (halo, juce::jmax (1.0f, geo.modStroke * 1.3f));
    }

    // 6. Label / value (cross-fades with hover).
    if (labelUpper.isNotEmpty())
    {
        const auto full = getLocalBounds().toFloat();
        const auto labelArea = juce::Rectangle<float> (full.getX(), kb.getBottom() + 2.0f, full.getWidth(), labelHeight());
        const float h = juce::jlimit (8.5f, hero ? 15.0f : 13.0f, labelArea.getHeight() * 0.7f);
        if (textLit > 0.02f)
            draw::trackedText (g, getTextFromValue (getValue()), labelArea, juce::Justification::centred, Theme::valueFont (h + 1.0f),
                               accent.darker (0.25f).withAlpha (textLit));
        if (textLit < 0.98f)
            draw::trackedText (g, labelUpper, labelArea, juce::Justification::centred,
                               draw::fitFont (Theme::labelFont (h), labelUpper, labelArea.getWidth() - 2.0f), Theme::textSecondary.withAlpha (1.0f - textLit));
    }
}

void AMKnob::visibilityChanged()
{
    // A knob hidden under the pointer never gets its mouse exit, so it clears here.
    if (! isShowing()) { hover.snap (0.0f); press.snap (0.0f); flare.snap (0.0f); dragging = false; }
}

void AMKnob::mouseEnter (const juce::MouseEvent& e) { anim.animate (hover, 1.0f); juce::Slider::mouseEnter (e); }
void AMKnob::mouseExit (const juce::MouseEvent& e)  { anim.animate (hover, 0.0f); juce::Slider::mouseExit (e); }

void AMKnob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showContextMenu();
        return;
    }
    // Assign mode swallows the click instead of starting a drag.
    if (modTarget.has_value() && ModAssign::get().assignTo (*modTarget, this))
        return;
    setMouseDragSensitivity (e.mods.isShiftDown() ? 1400 : 240);
    dragging = true;
    anim.animate (press, 1.0f);
    juce::Slider::mouseDown (e);
    repaint();
}

void AMKnob::mouseDrag (const juce::MouseEvent& e)
{
    setMouseDragSensitivity (e.mods.isShiftDown() ? 1400 : 240);
    juce::Slider::mouseDrag (e);
}

void AMKnob::mouseUp (const juce::MouseEvent& e)
{
    dragging = false;
    anim.animate (press, 0.0f);
    juce::Slider::mouseUp (e);
    repaint();
}

void AMKnob::showContextMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader (labelUpper);
    menu.addItem (1, "Reset to default");
    menu.addItem (2, "Enter value...");
    if (onContextMenu) onContextMenu (menu);

    juce::Component::SafePointer<AMKnob> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int result)
    {
        if (safe == nullptr) return;
        if (result == 1)
        {
            safe->setValue (safe->getDoubleClickReturnValue(), juce::sendNotificationSync);
        }
        else if (result == 2)
        {
            auto* box = new juce::AlertWindow ("Enter value", safe->label, juce::MessageBoxIconType::NoIcon);
            box->addTextEditor ("value", safe->getTextFromValue (safe->getValue()));
            box->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
            box->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            box->enterModalState (true, juce::ModalCallbackFunction::create ([safe, box] (int r)
            {
                if (r == 1 && safe != nullptr)
                    safe->setValue (safe->getValueFromText (box->getTextEditorContents ("value")), juce::sendNotificationSync);
            }), true);
        }
    });
}

} // namespace am::ui
