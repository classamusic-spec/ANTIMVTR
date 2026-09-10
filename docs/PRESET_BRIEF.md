# ANTI-MATR — Factory bank expansion brief

The library goes from **36 patches to 300**: twelve categories, **25 patches each**.
Three of each already exist and stay. Every category needs **+22 new patches**.

This document is the contract. Read `src/presets/FactoryBuilders.h` before writing
anything — it is the vocabulary every patch is written in.

## 1. Who owns what

Each author owns whole files. Nobody edits a file another author owns, so there
are no merge conflicts.

| author | files |
|---|---|
| bank-a | `src/presets/factory/Pad.cpp`, `Bass.cpp` |
| bank-b | `src/presets/factory/Keys.cpp`, `Pluck.cpp` |
| bank-c | `src/presets/factory/Lead.cpp`, `Texture.cpp` |
| bank-d | `src/presets/factory/Percussion.cpp`, `Drone.cpp` |
| bank-e | `src/presets/factory/Fx.cpp`, `Sequence.cpp` |
| bank-f | `src/presets/factory/Evolving.cpp`, `Cinematic.cpp` |

Do not touch `FactoryBuilders.h`, `FactoryContent.cpp`, the engine, the UI, or
any other author's category file. If you need a builder that does not exist,
say so in your report rather than adding one — a change there affects everyone.

## 2. What makes a patch worth shipping

A preset is not a set of parameter values. It is **a sound someone would choose
on purpose**. The test is simple and unforgiving: if a player auditions your
patch and the one before it and cannot say what is different, one of them should
not exist.

Distinctness has to be real, not nominal. Two patches differ meaningfully when
they differ in **at least two** of these:

* **Source** — which energy drives it (WAVE / DUST / IMPACT / SAMPLE / GESTURE),
  and how that source is set up.
* **Material and structure** — the material pair, blend, topology and coupling.
  A CRYSTAL lattice and a WOOD chain are different instruments.
* **Envelope shape** — a 2 ms attack and a 900 ms swell are different gestures
  even on the same material.
* **Register and pitch behaviour** — keytrack, Matter pitch, magnet target.
* **Movement** — what modulates what, and how fast. A patch that only moves
  because an LFO is on Density is not moving.
* **Space** — the reverb type, size and mix genuinely change the object's place.

Changing one knob by 0.1 and renaming it is padding. Do not do it. **22 excellent
patches beat 22 that fill a quota**, and a global similarity gate runs at
integration — near-duplicates come back to you.

Spread your 22 deliberately across the range of the category. Before you write,
sketch the 22 as a list of one-line intents ("a bowed glass pad that opens as
you hold it", "a sub that cracks like a struck plank") and make sure the list
itself reads as varied. Then build to the sketch.

## 3. Use the whole engine

The existing 36 lean on WAVE and IMPACT. The bank as a whole must exercise
everything, so across your 22:

* At least **4** built on SAMPLE or GESTURE sources.
* At least **4** with `sourceMode = LAYER` (two sources at once).
* At least **6** with Fracture on and doing something structural.
* At least **4** where an Evolve operator (bend, melt, tear, magnet, crush,
  freeze) is the reason the patch is interesting.
* At least **8** with per-voice modulation — velocity, key track, note random —
  so the patch responds to playing rather than sitting still.
* Use all nine materials and all six topologies somewhere in your set.

## 4. Levels and the category gate

Every patch is rendered (one note, held 2 s, 1.5 s tail) and must land inside
its category's RMS and peak window — see `categories()` in
`src/presets/FactoryContent.cpp`. Outside the window it fails the build.

Aim for the **middle** of the window, not the edge. A patch at the ceiling
clips as soon as a player holds a chord. Check with:

```
scripts/render.sh --preset "Your Patch" --seconds 3.5 --json
```

and read `peak`, `rms`, `dc` and `safety`. **Any non-zero safety counter is a
failure** — limiting included. Test at notes 36, 60 and 84: a patch that only
behaves in the middle of the keyboard is not finished.

## 5. Names

Two words, evocative, no vendor clichés ("Super Saw 3", "Bass 07"). The name
should tell you something the category does not: `Bone Marimba`, `Torn Bass`,
`Event Horizon`. No numeric suffixes and no reuse of an existing name — check
`git grep 'addFactory' src/presets/factory/` first. Names are permanent once
shipped, so they are worth a minute each.

## 6. Tags — a closed vocabulary

The browser shows one chip per distinct tag. Free-form tagging gives 300 chips
and an unusable browser, so tags come **only** from this list. Use three to five
per patch, drawn from at least two groups.

* **Character** — `dark` `bright` `warm` `cold` `clean` `dirty` `soft` `harsh`
  `metallic` `wooden` `glassy` `organic` `synthetic` `hollow` `vocal`
* **Motion** — `static` `breathing` `pulsing` `evolving` `rhythmic` `chaotic`
  `morphing` `unstable`
* **Space** — `dry` `close` `roomy` `wide` `huge` `distant`
* **Register** — `sub` `low` `mid` `high` `air`
* **Gesture** — `struck` `plucked` `bowed` `blown` `granular` `noisy`
  `resonant` `formant` `scraped`
* **Use** — `chords` `melodic` `layer` `intro` `transition` `impact` `drift`

## 7. Macros are a promise

MACRO 5 is always LENGTH and MACRO 6 is always CHAOS — a player who learns them
on one patch knows them on all 300. Call `sharedMacros (r, ...)` on every patch.
Macros 1–4 are MOTION / BRIGHT / SPACE / CHARACTER; wire them so each does
something audible. A macro that moves nothing is a bug.

## 8. Before you report

1. `scripts/build.sh` — clean.
2. `scripts/test.sh factory` — green. (It is slow; run it once at the end, not
   between patches.)
3. Render **every** patch you wrote at notes 36, 60 and 84 and read the numbers.
   Do not report a patch you have not rendered.
4. Commit in batches of about eight so progress survives an interruption.

Report: the list of patches with a one-line intent each, the range you covered,
anything you could not make work, and any builder you wanted and did not have.
