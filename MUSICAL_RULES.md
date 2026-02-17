# Musical Rules

Reference document for all musicality overrides applied on top of the cellular automata algorithms.
These rules keep the generative/pseudorandom nature intact while steering output toward musical results.

---

## 1. Scale Quantization

**Where**: `ScaleQuantizer::quantize()` (applied to every triggered note)
**Param**: `scale` (13 scales), `key` (12 root keys)
**Rule**: Raw grid position (row, col) is mapped to a MIDI note within the selected scale. No chromatic notes escape -- every trigger is forced into the chosen scale.

---

## 2. Pitch Gravity

**Where**: `ScaleQuantizer::quantizeWeighted()` in processBlock
**Param**: `pitchGravity` (0.0 - 1.0, default 0.3)
**Rule**: Biases pitch selection toward chord tones (root, 5th, 3rd of the current scale). At `gravity=1.0`, every note snaps to a chord tone. At `gravity=0.0`, normal quantization applies. Probabilistic -- each note rolls independently.

---

## 3. Consonance Filter

**Where**: `ScaleQuantizer::isConsonantWithAll()` in processBlock
**Param**: `consonance` (0.0 - 1.0, default 0.5)
**Rule**: Before a new note sounds, its interval against ALL currently-sounding notes is checked. Dissonant intervals (m2, M2, tritone, m7, M7) trigger probabilistic rejection. At `consonance=1.0`, dissonant notes are always rejected. At `consonance=0.0`, filter is bypassed.

**Consonant intervals**: unison, m3, M3, P4, P5, m6, M6, octave
**Dissonant intervals**: m2, M2, tritone, m7, M7

---

## 4. Note Probability

**Where**: processBlock column loop
**Param**: `noteProbability` (0.0 - 1.0, default 0.5)
**Rule**: Each activated cell has a chance to be skipped. Roll < noteProb = trigger, roll >= noteProb = skip. Lower values = sparser output. This is per-column, so skipping one column doesn't affect the next.

---

## 5. Rest Probability

**Where**: processBlock, before the column loop
**Param**: `restProbability` (0.0 - 1.0, default 0.2)
**Rule**: Before any notes are triggered for a given engine step, roll once. If roll < restProb, the entire step is silenced (full rest). Creates rhythmic breathing without touching the CA state. At default 0.2, roughly 1 in 5 steps is silent.

---

## 6. Max Triggers Per Step

**Where**: processBlock column loop, `triggersThisStep` counter
**Param**: `maxTriggersPerStep` (1 - 8, default 3)
**Rule**: Hard cap on how many new voices can start per engine step. Even if 16 cells activate simultaneously, only N will actually trigger. Prevents walls of simultaneous onsets.

---

## 7. Density-Adaptive Voice Count

**Where**: processBlock, before column loop
**Param**: Automatic (no user param -- driven by grid state)
**Rule**: When grid density exceeds 30%, the effective max voice count is linearly reduced. At 100% density, effective voices = 50% of max. Prevents combinatorial explosions when CA patterns become dense. Formula:
```
if density > 0.3:
  reduction = (density - 0.3) / 0.7 * 0.5
  effectiveMax = max(1, maxVoices - (reduction * maxVoices))
```

---

## 8. Density-Driven Dynamics

**Where**: processBlock, applied to all voices
**Param**: Automatic (driven by grid state)
**Rule**: Grid density modulates two things:
- **Gain**: Dense grids play softer (density 0.0 -> gain 1.0, density 1.0 -> gain 0.35)
- **Filter cutoff**: Dense grids open the filter wider (density 0.0 -> cutoff x0.5, density 1.0 -> cutoff x1.0)

This creates a natural "swell" as patterns grow, and keeps sparse patterns intimate/dark.

---

## 9. Melodic Inertia

**Where**: processBlock, pitch decision
**Param**: `melodicInertia` (0.0 - 1.0, default 0.5)
**Rule**: Chance to reuse the last triggered MIDI note instead of computing a new one from grid position. Creates melodic repetition and motifs. At `inertia=1.0`, the same pitch repeats indefinitely. At `inertia=0.0`, every trigger computes fresh.

---

## 10. Velocity Humanization

**Where**: processBlock, after pitch decision
**Param**: `velocityHumanize` (0.0 - 1.0, default 0.05)
**Rule**: Random offset applied to velocity (MIDI velocity -> amplitude). Offset is +/- `velHumanize` around the base velocity. Clamped to [0.1, 1.0]. Prevents mechanical, uniform dynamics.

---

## 11. Engine-Specific Intensity

**Where**: `CellularEngine::getCellIntensity()` overrides in processBlock
**Param**: None (automatic per engine type)
**Rule**: Continuous engines (Lenia, Reaction-Diffusion, Particle Swarm, Brownian Field) return native float state as intensity [0.0 - 1.0]. This modulates voice velocity, giving each engine a natural dynamic contour:
- **Lenia**: state field value
- **Reaction-Diffusion**: fieldB concentration
- **Particle Swarm**: trail value at cell
- **Brownian Field**: energy value at cell

Binary engines (Game of Life, Brian's Brain, Cyclic CA) return 1.0 (full intensity).

---

## 12. Gate Time

**Where**: `SynthVoice`, driven by `gateRemainingSamples`
**Param**: `gateTime` (0.0 - 1.0, default 0.8)
**Rule**: Controls note duration as fraction of the step interval. At `gateTime=1.0`, notes sustain for the full step. At `gateTime=0.5`, notes are staccato (50% duty cycle). Auto-noteOff fires when the gate timer expires.

---

## 13. Strum Spread

**Where**: `SynthVoice`, driven by `onsetDelaySamples`
**Param**: `strumSpread` (0 - 50ms, default 0ms)
**Rule**: Per-column onset delay. Column 0 plays immediately, column N plays after `(col / maxCols) * strumSpread` ms of silence. Creates strummed/arpeggiated spread across the grid. At 0ms, all columns are simultaneous.

---

## 14. Cell Death = Voice Release

**Where**: processBlock, voice release loop
**Param**: None (automatic)
**Rule**: Each active voice tracks its grid position (row, col). When the corresponding cell dies (state -> 0), the voice receives noteOff. This keeps the audio tightly coupled to the CA state -- sounds only persist while their cell is alive.

---

## 15. Voice Stealing (Quietest-First)

**Where**: processBlock, voice allocation
**Param**: None (automatic)
**Rule**: When all voices are busy and a new trigger arrives, the quietest currently-active voice is stolen. This ensures new events are heard while minimizing audible disruption.

---

## Design Philosophy

These rules form a hierarchy of musical control:

1. **Hard constraints** (always applied): Scale quantization, cell-death release, voice stealing
2. **Shaping controls** (user-adjustable): Gate time, consonance, pitch gravity, max triggers
3. **Texture controls** (user-adjustable): Note probability, rest probability, melodic inertia, strum spread
4. **Automatic adaptation** (no user param): Density-driven dynamics, density-adaptive voices, engine intensity

The goal is that **any combination of CA algorithm + parameters produces something listenable**, while still reflecting the underlying emergent behavior of the automaton. The rules constrain the output without overriding the generative source -- they filter, shape, and humanize rather than replace.
