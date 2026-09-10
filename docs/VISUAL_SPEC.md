# ANTI-MATR — Visual Specification (reference target)

This document describes, element by element, the look the editor must reproduce.
It exists because the reference image cannot be checked into the repository: this
text **is** the target. Everything here is drawn procedurally in JUCE from
component bounds — no bitmaps, no external assets, no image files (SPEC §6).

The overall impression is a **machined physical instrument**: dark anodised
metal panels bolted to a chassis, soft moulded rubber knobs lit from above, and
at the centre a thick curved **glass porthole** with an energy object alive
behind it. Nothing is flat. Every surface has a light source at the top-left and
casts a soft shadow at the bottom-right.

## 1. Chassis and panels

* Background: near-black charcoal, very slightly warm, with a fine procedural
  noise/grain so large areas never band. A subtle vignette darkens the corners.
* Panels are **raised slabs**, not outlines: rounded rectangles (radius ≈ 1.2 %
  of the editor width) filled with a vertical gradient from a lighter top to a
  darker bottom, with
  * a 1 px light edge along the top and left (the highlight),
  * a 1–2 px black edge along the bottom and right (the shadow),
  * a soft drop shadow beneath the whole slab,
  * a barely visible brushed-metal streak texture running horizontally.
* Each panel carries **four small screws/rivets**, one inset from each corner:
  a dark circle with a bright crescent on the upper-left and a slot or cross cut
  into it. They are small (≈ 6–8 px at the reference size) and quiet.
* Panel titles sit top-left in letterspaced uppercase, bright; the subtitle sits
  directly under them in a dimmer, smaller weight. A short accent underline in
  the section colour sits under the title block.
* Inner displays (waveform, spectrum, step editor, nebula thumbnail) are
  **inset** rather than raised: the same rounded rectangle with the gradient and
  edges inverted, so they read as recessed screens behind glass.

## 2. Knobs — neuromorphic, physical

The single most important control. Each knob is built in layers, outside in:

1. **Outer arc ring.** A thin ring set slightly outside the knob body, drawn as
   an arc from roughly 7 o'clock to 5 o'clock. The unfilled part is very dark;
   the filled part is a **two-stop gradient** in the section's accent pair (for
   example blue → violet, violet → magenta, cyan → blue), with a soft outer glow
   that grows with the value. The arc has rounded caps.
2. **A dark gap** between ring and body so the ring reads as separate hardware.
3. **The body**: a circle filled with a radial gradient that is lighter at the
   top-left and darker at the bottom-right, giving a moulded, slightly domed
   look. A subtle rim runs around the body: light at the top, dark at the
   bottom, like a machined edge.
4. **A specular highlight**: a soft elliptical bloom in the upper-left third,
   low opacity, giving the rubber/metal sheen.
5. **The indicator**: a single crisp near-white line from about 55 % of the
   radius to just inside the rim, with a faint glow, rotating with the value.
   It is the brightest thing on the knob.
6. **Contact shadow** under the body so it sits on the panel rather than in it.

Hover raises the specular highlight and the ring glow slightly. Drag does the
same, a little more. Modulated knobs additionally show a **second, thinner arc**
just outside the value ring, in the modulation colour, spanning the range the
modulation covers, with a small dot at the live modulated position.

Labels sit centred below in letterspaced uppercase, dim; the value appears in
place of the label while the knob is being moved or hovered.

## 3. Sliders

Horizontal capsule track, recessed (dark inside, light lower edge). The filled
portion is a left-to-right gradient in the section accent pair with a soft glow.
The handle is a small circular knob built like a miniature knob body (radial
gradient, rim light, contact shadow). The numeric value sits right-aligned at
the end of the row in a dim monospaced-feeling face.

## 4. Toggles and segmented controls

* **Toggle**: a recessed capsule; when on, it fills with the accent gradient and
  the circular handle slides right with a glow; when off it is dark with the
  handle left. A label sits beside or beneath in small letterspaced caps.
* **Segment** (SIMPLE / ADVANCED, OFF / ON, A / B): a recessed capsule split
  into cells; the selected cell is a raised slab with an accent fill and a soft
  glow, the others are flat and dim.

## 5. The centre — glass porthole

This is the identity of the product and deserves the most work.

* A **circular metal bezel** occupies the centre column: a thick ring drawn as a
  brushed-metal gradient (light at the top, dark at the bottom, with a second
  narrow highlight arc near the top edge so it reads as curved). Around it, at
  regular intervals, sit **bolt heads** in the same style as the panel screws.
  The bezel has an outer shadow on the panel behind it and an inner shadow onto
  the glass.
* Left and right of the bezel, two short **vertical light bars** glow warm
  amber-white, like the instrument's status lamps, each with a soft bloom.
* Inside the bezel sits the **glass**: after the object is drawn, the glass is
  composited over it as
  * a broad diagonal specular sweep from the upper-left, low opacity, white,
  * a tighter crescent highlight hugging the inside of the bezel at the top,
  * a faint darkening toward the lower-right inside edge (thickness),
  * a very subtle chromatic fringe at the extreme edge,
  * optional slow-moving faint smudges so the glass feels real, never dirty.
  The glass must never wash out the object: it is a highlight layer, not a haze.
* Behind the glass, the **ANTI-MATTER object**: a roughly spherical mass of
  bright filaments — long, smooth, curving strands in blue, violet and magenta
  that orbit and weave around a **dark, almost black core**. The strands are
  brightest where they cross in front of the core and fade where they pass
  behind it, which is what sells the depth. Small bright nodes sit where strands
  meet. Around and between them float **small translucent spheres/bubbles** and
  fine particles, drifting slowly, catching the light. The whole mass rotates
  slowly and breathes with the audio.
* The object reacts to the engine: overall size and brightness follow level,
  strand count and density follow Shape density, the core opens and closes with
  mass, the strands deform and tear with the Evolve operators, and the particles
  scatter on Fracture.
* Below the bezel sits a **plinth**: a wide, shallow metal base with a brushed
  gradient and a bright rim, carrying the engraved wordmark **ANTI-MATR** and
  under it **SOUND BEYOND MATTER** in small letterspaced caps. A glowing arc of
  accent light spills from under the plinth onto the panel.
* Small labels flank the top of the porthole: `INHALE / IDEA` on the left,
  `EXHALE / EVOLVE` on the right, dim and letterspaced.

## 6. Source selector

A row of **circular glass buttons**, one per energy source. Each is a small
sphere: a dark ball with a radial gradient, a bright specular dot in the upper
left, a rim light, and inside it a small procedural icon that says what the
source is (a waveform, a dust cloud, an impact starburst, a sample waveform, a
gesture curve). The selected one gains a bright accent ring around the sphere
and a stronger glow; the others are dimmer and slightly smaller. Labels sit
below in letterspaced caps.

## 7. Displays

* **Waveform**: an inset screen with a glowing polyline in the source accent
  colour, a soft gradient fill under it, a faint grid, and small chevrons at
  the left and right edges for stepping through banks/frames.
* **Fracture spectrum**: an inset screen with a jagged polygonal skyline in two
  colours — cool blue on the left half moving to magenta on the right — each
  with a translucent fill and a bright top edge, over a faint grid.
* **Nebula thumbnail**: an inset rounded rect containing a generated spiral
  galaxy: a bright core, a soft violet halo, and fine star specks along the
  arms.

## 8. Typography

One condensed, slightly technical sans throughout, always uppercase for labels,
with generous letterspacing. Four sizes only: panel title, panel subtitle,
control label, and value. The wordmark is heavier, with a metallic vertical
gradient and a fine bevel: light top edge, dark bottom edge.

## 9. Colour

* Chassis: near-black, warm charcoal.
* Panel: dark grey-blue slabs.
* Text: near-white for titles and values, mid-grey for labels, dim grey for
  captions.
* Accents, one pair per section, used on that section's arcs, fills and glows:
  * SOURCE — blue → cyan
  * SHAPE — cyan → violet
  * EVOLVE — violet → indigo
  * FRACTURE — magenta → violet
  * SPACE — ivory → blue
  * MOD — amber → magenta
* Glow is always the accent at low opacity, never white, and always means
  something: value, activity or selection.

## 10. Rules that do not bend

* Everything derives from `getLocalBounds()`. No absolute pixel constants: the
  editor must look identical in proportion at 1100×690 and at 1600×1000.
* No bitmaps, no fonts or images loaded from disk, no external assets.
* Nothing paints outside its component. No tooltip or popup may render unless
  the pointer is actually over its owner.
* Every shadow, highlight and glow is drawn from the same top-left light
  direction. Consistency is what makes it read as one machined object.
* The centre object animates continuously but must cost little: it runs at
  30 Hz on the message thread and reads only the diagnostic snapshots.
