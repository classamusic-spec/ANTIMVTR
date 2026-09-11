# ANTI-MATR — Visual Specification (reference target)

**This document replaces the previous dark-chassis specification.** The
instrument has pivoted to a light, pearl-and-silver aesthetic. Everything here
is drawn procedurally in JUCE from component bounds — no bitmaps, no external
assets, no image files.

The overall impression is **premium laboratory hardware**: a pale machined
chassis, panels of frosted white glass floating on it with soft shadows
underneath, dark moulded knobs with turned-metal caps, and recessed dark
displays showing luminous line art. Think a high-end instrument photographed on
a white desk, not a plugin. Nothing is flat, and the light always comes from
above and slightly to the left.

## 1. Chassis and panels

* Background: a cool pearl grey, very slightly lighter toward the top, with a
  fine procedural grain so large areas never band. No vignette — the ground is
  light and open.
* Panels are **frosted glass slabs floating above the chassis**, not cut into
  it. Rounded rectangles (radius ≈ 1.2 % of editor width), filled with a nearly
  white vertical gradient, and given depth by three things only:
  * a **soft drop shadow** beneath and slightly right — this is what does the
    work on a light ground,
  * a 1 px near-white highlight along the top and left edge,
  * a faint dark hairline along the bottom and right.
  A panel must never be *darker* than the chassis. Depth comes from the shadow.
* A very fine frost texture across the panel face — a whisper, not a haze.
* Panel titles sit top-left in letterspaced uppercase, near-black; the subtitle
  sits under them in mid-grey. A short accent underline in the section colour
  sits under the title block.
* Inner displays (waveform, spectrum, step editor, nebula, material lattice)
  are the **one dark element on the page**: deeply recessed near-black screens
  with an inner shadow along their top edge, carrying luminous line art. The
  contrast between pale chassis and dark screen is the signature of the design.

## 2. Knobs — three styles, one family

The reference shows three variants that share one body. Every knob in the
instrument is one of these.

**A. Capped, with an LED ring** — the hero control.
1. A **ring of small round LED dots** set outside the knob, spanning roughly
   7 o'clock to 5 o'clock. Dots below the value are **lit amber-orange** with a
   soft bloom that spills onto the panel; dots above it are unlit — dark holes
   with a faint inner shadow, as if drilled.
2. A dark gap, then the **body**: a matte near-black moulded ring with a soft
   outer bevel, lighter at the top-left, darker at the bottom-right, and a
   contact shadow beneath it.
3. Inset into the body, a **turned metal cap**: a silver disc with a fine
   **radial sunburst brush** — narrow wedges of alternating light and shade
   radiating from the centre, brightest toward the upper-left. A crisp bright
   rim runs around the cap.
4. The **indicator**: a fine dark line cut from the top of the cap outward
   across the body, rotating with the value.

**B. Capped, ring unlit** — the same knob with every dot dark. Used where a
control is inactive or where a dense cluster would be noisy if all rings glowed.

**C. Plain** — no metal cap and no dots: a soft, slightly larger matte dome with
a subtle concentric groove near its edge and a small notch at the top. Quiet;
used for secondary controls.

**Lighting and animation.** The cap's sunburst highlight rotates with the value,
so turning the knob makes the metal catch the light differently — this is the
detail that sells it. Hovering lifts the cap's specular and warms the lit dots
slightly. On a value change the newly lit dot flares brighter for a moment and
settles. A modulated knob shows a second, thinner arc of dots just outside the
ring in the modulation colour, with a brighter dot at the live modulated
position. All of it is continuous and eased — nothing snaps.

## 3. Sliders, toggles, segments

* **Slider**: a recessed light-grey capsule track with a soft inner shadow; the
  filled portion is a gradient in the section accent; the handle is a miniature
  version of knob body A — dark, capped, with a rim light.
* **Toggle / segment** (SIMPLE / ADVANCED, OFF / ON, A / B): a recessed capsule
  split into cells; the selected cell is a raised white slab with the accent as
  a thin underline or fill and a soft glow; the others sit flush and dim.
* **Sidebar pills** (the vertical lists on the SOURCE, SHAPE, EVOLVE, FRACTURE
  and SPACE pages): rounded rectangles with a small glyph and a label; the
  selected pill is a raised white slab with an accent left-edge and a soft
  shadow, the rest flat and quiet.

## 4. The centre — glass sphere

The MAIN page's centre is the identity of the product and the largest element
on the page.

* A **thick circular bezel** of brushed silver, segmented into plates with fine
  seams and bolt heads at the seams, catching the light along its upper edge.
  Two small warm status lamps sit at nine and three o'clock.
* Inside it, the **glass**: a broad diagonal specular sweep from the upper left,
  a tighter crescent hugging the inside of the bezel at the top, a faint
  thickness darkening toward the lower right, and a subtle chromatic fringe at
  the extreme edge. The glass is a highlight layer, never a haze.
* Behind the glass, the **ANTI-MATTER object** on a dark well, so it reads as a
  lit interior seen through clear glass. The object is driven by the engine's
  own modal data — see §5.
* Beneath the sphere, the wordmark **ANTI-MATR** in letterspaced caps. No
  plinth, no stand: the sphere floats.

## 5. The object reacts to what is played

`VisualStateSnapshot` carries up to 32 modal resonators with individual
frequency, energy, pan and cluster membership — the actual physical model making
the sound — plus note events (`noteId`, velocity, whether the key is held),
`fractureHits`, the Evolve movement clock, and every sounding voice's pitch and
energy. The object must be bound to these, not to a generic level meter:

* size and brightness follow level; ribbon/particle count follows Density
* the dark core opens with Mass; Tension tightens the orbits; Surface roughens
* each Evolve operator deforms it in its own way (bend, melt, tear, magnet,
  crush, freeze, scatter)
* a note-on throws a visible burst — from `noteId` changing, not the envelope
* a chord looks different from a single note (`numVisualVoices`)
* it breathes at rest: the instrument must never look asleep

## 6. Page layouts

Every page is: a left sidebar or selector column, a large dark display in the
centre, and control clusters on the right. The bottom navigation bar is
constant.

* **MAIN** — SOURCE panel top-left (source selector row, waveform strip, four
  knobs); the glass sphere centred and dominant; SHAPE top-right (dark lattice
  display, knobs); EVOLVE bottom-left (four operator controls); SPACE
  bottom-right (preset selector, four knobs); FRACTURE as a strip beneath the
  sphere.
* **SOURCE** — vertical pill list (WAVE / DUST / IMPACT / SAMPLE / GESTURE) on
  the left; a large dark display showing the wavetable as a luminous mesh with
  a table selector beneath it; two rows of three knobs on the right; stepper
  rows for TABLE / UNISON and OCTAVE / SEMITONE / FINE.
* **SHAPE** — sidebar SIMPLE / ADVANCED / MATERIAL; centre display showing the
  material lattice with a blend selector; two rows of three knobs (DENSITY FORM
  MASS / TENSION DECAY SURFACE).
* **EVOLVE** — sidebar MAIN / ADVANCED / MOTION; centre display showing the
  deformation as a flowing surface; four operator knobs and AMOUNT / SPEED
  sliders.
* **FRACTURE** — sidebar MAIN / SEQUENCER / FRAGMENTS; centre display showing
  the shattered object; a mode selector, a bar spectrum and four knobs.
* **SPACE** — vertical list of space types; centre display showing the space as
  a generated nebula; four knobs and a SPACE ENGINE row of module toggles
  (DIFFUSION / DELAY / REVERB / SPECTRAL / WIDTH / COMP).
* **Bottom navigation** — a light bar with a glyph and label per page; the
  active page is a raised white pill with an accent underline. A / B and the
  morph control sit at the right.

## 7. Typography

One condensed technical sans throughout, uppercase for labels, generous
letterspacing. Four sizes only: panel title, panel subtitle, control label,
value. Near-black for titles and values, mid-grey for labels, light grey for
captions. The wordmark is heavier with a fine engraved bevel — a light top edge
and a dark lower edge cut into the pale ground.

## 8. Colour

* Chassis: cool pearl grey. Panels: frosted near-white. Displays: near-black.
* Text: near-black, mid-grey, light grey.
* Accents, one pair per section, deepened so they read against a pale ground:
  SOURCE blue→cyan, SHAPE cyan→violet, EVOLVE violet→indigo, FRACTURE
  magenta→violet, SPACE ivory→blue, MOD amber→magenta.
* The LED dot ring is **amber-orange** on every knob regardless of section — it
  is the instrument's one warm colour and it is what the eye lands on.

## 9. Rules that do not bend

* Everything derives from `getLocalBounds()`. No absolute pixel constants: the
  editor must look identical in proportion at 1100×690 and at 1600×1000.
* No bitmaps, no fonts or images loaded from disk, no external assets.
* Nothing paints outside its component. No tooltip or popup may render unless
  the pointer is actually over its owner.
* Every shadow, highlight and glow is drawn from the same top-left light
  direction.
* The centre object animates continuously but cheaply: 30 Hz on the message
  thread, reading only the diagnostic snapshots, under 6 ms per frame at
  1600×1000.
