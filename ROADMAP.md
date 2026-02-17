# Algo Nebula — Development Roadmap

> Multi-Algorithm Generative Ambient Synthesizer
> JUCE VST3/Standalone Plugin

---

## Cross-Cutting Concerns

These apply to EVERY phase and are not optional.

### Logging Infrastructure (from Phase 2 onward)
- `juce::Logger` with file output + console output
- Log levels: debug / info / warn / error
- Entry/exit logging for `prepareToPlay()`, `releaseResources()`, algorithm swaps
- Startup diagnostics: version, OS, audio device, sample rate, buffer size, loaded config
- State transition logging: algorithm changes, transport events, preset loads
- Boundary logging: MIDI I/O, parameter changes, grid state saves
- Use `#ifdef DEBUG_VERBOSE` for per-sample/per-block internals
- All debug logging stripped in Release builds

### Documentation Updates (every phase)
- `CHANGELOG.md` updated in the SAME commit as the feature
- `STATE.md` updated when features land
- `ARCHITECTURE.md` updated when component structure changes
- `HOWTO.md` updated when build process or dev workflow changes

### Build Preservation (every build)
- Discover actual output paths dynamically before building
- Copy all build artifacts to `releases/YYYY-MM-DD_HHMM/` before each build
- Tag working builds in git after tests pass

### Technical Debt Reviews
Explicit pause-and-review checkpoints:
- **After Phase 5** (first playable): abstractions holding? test gaps?
- **After Phase 10** (all algorithms): performance debt? code duplication across algorithms?
- **After Phase 15** (pre-release): final cleanup pass, remove all `// TODO` and `// HACK`

### State Format Versioning
- State blob includes a version number (uint32) as the first field
- Migration functions: `migrateStateV1toV2()`, etc.
- Forward-compatible: unknown fields are preserved, not discarded
- Roundtrip test runs against saved state blobs from every tagged version

### Defensive Coding for DSP
- All algorithm outputs clamped to valid range before downstream use
- NaN/Inf guard on every algorithm `step()` output (replace with 0.0)
- Grid values bounds-checked on read (not just write)
- Feed/kill rates for R-D validated on parameter change
- Lenia growth function output clamped to prevent runaway
- Brownian walker positions wrapped to grid bounds

---

## Phase 1 — Skeleton + Thread Safety + UI Foundation (`v0.1.0`) :white_check_mark:

**Goal:** Build system, dependencies, basic UI chrome. First successful build.

- [x] CMake + CPM: JUCE 8.0.4, DaisySP, Signalsmith, melatonin_blur
- [x] Embed Inter + JetBrains Mono fonts as BinaryData
- [x] `NebulaLookAndFeel` + `NebulaColours.h` with deep-space theme
- [x] Empty processor with APVTS stub (scale, key, algo, waveform selectors)
- [x] Build VST3 + Standalone, git init, mandatory docs
- [x] Fix: signalsmith include path shadowing Windows SDK `<windows.h>`

**Testing Milestone:**
- [x] Clean build (0 errors, 0 warnings in project code)
- [ ] Headless test target compiles and runs (empty test passes)
- [ ] Standalone launches without crash

**RT Safety Checkpoint:**
- [ ] No allocations in `processBlock()` (currently trivial — just silence output)
- [ ] `SmoothedValue` infrastructure stubbed for all continuous params

**Tag:** `v0.1.0` after all items complete

---

## Phase 2 — CellularEngine + Game of Life (`v0.2.0`)

**Goal:** First cellular automaton running in the audio thread with double-buffered grid.

- [x] Set up logging infrastructure (juce::Logger, file + console, log levels)
- [x] `CellularEngine` abstract interface (`step()`, `getGrid()`, `reset()`, `seed()`)
- [x] `GameOfLife` implementation
  - 5 rule presets: Classic (B3/S23), High Life, Day & Night, Seeds, Ambient
  - Toroidal wrapping (edges connect)
  - Cell age tracking (generations alive)
  - Seeded random initialization (`uint64_t seed`)
- [x] Double-buffered grid swap mechanism (lock-free, audio owns front, GL reads back)
- [x] SPSC queue for UI -> audio cell edit commands

**Testing Milestone — Correctness:**
- [ ] GoL Blinker oscillates correctly (period 2, exact cell positions verified)
- [ ] GoL Glider translates correctly (period 4, displacement verified)
- [ ] GoL still-life patterns (Block, Beehive, Loaf) remain stable for 1000 generations
- [ ] Seeded init produces identical grids for same seed + density (bit-perfect)
- [ ] All 5 rule presets produce distinct steady-state behavior (statistical cell count divergence)
- [ ] Toroidal wrapping: glider crossing edge reappears on opposite side at correct position
- [ ] Cell age increments correctly (tracked per-cell, reset on death, verified at known generations)

**Testing Milestone — Integration:**
- [ ] Double-buffer swap: audio thread writes don't corrupt GL thread reads (concurrent stress test)
- [ ] SPSC queue: 1000 rapid cell edits from UI thread drain correctly in processBlock

**Testing Milestone — Mutation:**
- [ ] Mutate neighbor count threshold (B3 -> B2): Blinker test MUST fail
- [ ] Mutate survival rule (S23 -> S2): Blinker test MUST fail (dies on gen 2)
- [ ] Swap toroidal wrap to clamp: Glider-crossing-edge test MUST fail
- [ ] Remove age increment: Age tracking test MUST fail
- [ ] Corrupt seed PRNG: Seeded-identity test MUST fail
- [ ] Mutation survival rate < 20%

**RT Safety Checkpoint:**
- [ ] `CellularEngine::step()` is O(rows * cols), no allocations
- [ ] Double-buffer swap is a single atomic pointer exchange
- [ ] SPSC queue drain in `processBlock()` is bounded (max N commands/block)
- [ ] Grid storage is pre-allocated at max size in `prepareToPlay()`

**Tag:** `v0.2.0`

---

## Phase 3 — Scale Quantizer + Clock + Microtuning (`v0.3.0`)

**Goal:** Musical pitch mapping and tempo-synced stepping.

- [x] `ScaleQuantizer`: 15 scales x 12 root keys, octave range (base + span)
- [x] `Microtuning`: 12-TET, Just Intonation, Pythagorean, adjustable A4 reference (420-460Hz)
- [x] `ClockDivider`: host transport sync, free-running internal clock, swing

**Testing Milestone — Correctness:**
- [ ] All 15 scales produce mathematically correct interval sets (verified against music theory reference)
- [ ] All 12 root transpositions shift intervals correctly (not just C-based)
- [ ] Microtuning 12-TET: A4 = 440Hz exactly, A3 = 220Hz exactly
- [ ] Microtuning Just Intonation: P5 = 3/2 ratio (701.955 cents, verified ± 0.01 cent)
- [ ] Microtuning Pythagorean: P5 = 3/2 ratio, M3 = 81/64 (407.82 cents)
- [ ] A4 reference adjustment: 432Hz produces 432.0Hz ± 0.001Hz for A4
- [ ] Clock fires correct number of steps for given BPM + division + buffer size (sample-accurate)
- [ ] Swing timing: 67% swing offsets every other step by exactly (2/3 - 1/2) * step_duration

**Testing Milestone — Integration:**
- [ ] Clock -> GoL stepping: at 120BPM 1/4 note, GoL steps exactly 2 times per second
- [ ] Scale quantizer with GoL: active cells map to valid scale degrees only (no out-of-scale notes)
- [ ] Transport stop: GoL pauses, grid state frozen, resume continues from same state

**Testing Milestone — Mutation:**
- [ ] Change Dorian mode interval [2,1,2,2,2,1,2] -> [2,1,2,2,2,2,1]: scale test MUST fail
- [ ] Offset Just Intonation P5 ratio by 1 cent: tuning test MUST fail
- [ ] Change clock sample counter from `>=` to `>`: step count test MUST fail
- [ ] Mutation survival rate < 20%

**RT Safety Checkpoint:**
- [ ] Scale lookup is array index (O(1)), no search
- [ ] Microtuning tables pre-computed in `prepareToPlay()`, not per-sample
- [ ] Clock uses integer sample counting, no floating-point drift accumulation
- [ ] No string formatting or logging in clock tick path

**Tag:** `v0.3.0`

---

## Phase 4 — PolyBLEP Synth Voices (`v0.4.0`)

**Goal:** Band-limited oscillators, envelopes, filters, sub-oscillator. Produces sound.

- [x] `PolyBLEPOscillator`: 8 waveshapes (sine, triangle, saw, pulse, sine+oct, fifth stack, pad, bell)
- [x] `SynthVoice`: AHDSR envelope, SVF filter (LP/HP/BP/Notch), noise layer
- [x] Sub-oscillator: sine, follows lowest voice, -1/-2 octave, level knob
- [x] Per-voice pan + auto-pan from grid column position
- [ ] Establish **CPU performance baseline**: single voice at 44.1kHz/512 samples

**Testing Milestone — Correctness (Oscillators):**
- [ ] Sine: output matches `std::sin()` reference to < -120dB error
- [ ] Sine: FFT shows single peak at fundamental, all harmonics < -100dB
- [ ] Saw: FFT harmonics follow 1/n amplitude law (± 1dB for first 20 harmonics)
- [ ] Saw: no aliasing — all energy above Nyquist/2 is < -60dB
- [ ] Square: FFT odd harmonics follow 1/n, even harmonics < -60dB
- [ ] Pulse: width = 0.5 matches square, width = 0.25 has correct harmonic structure
- [ ] Triangle: FFT odd harmonics follow 1/n², even harmonics < -60dB
- [ ] PolyBLEP correction: saw at 10kHz (44.1kHz SR) has < -60dB aliasing vs naive saw
- [ ] Bell (FM): output is non-zero, inharmonic spectrum (not integer harmonics)
- [ ] Pad: detuned unison produces expected beating rate (± 0.5Hz for ± 7 cent detune)
- [ ] All waveshapes produce output in [-1.0, 1.0] range (no clipping, no DC offset > 0.001)
- [ ] Frequency accuracy: 440Hz request produces 440.0Hz ± 0.01Hz (measured via zero-crossing)

**Testing Milestone — Correctness (Envelope):**
- [ ] Attack ramp: linear ramp from 0 to 1 over specified time (± 1 sample)
- [ ] Hold: sustains at 1.0 for specified duration (± 1 sample)
- [ ] Decay: ramps from 1.0 to sustain level over specified time
- [ ] Sustain: holds at specified level indefinitely
- [ ] Release: ramps from current level to 0 over specified time (± 1 sample)
- [ ] Note-off during attack: transitions to release from current value (no discontinuity > 0.01)
- [ ] Retrigger during release: restarts from current value (no click)

**Testing Milestone — Correctness (Filter):**
- [ ] SVF LP at 1kHz: -3dB at cutoff, -12dB/oct rolloff (± 1dB at 2kHz, 4kHz)
- [ ] SVF HP at 1kHz: -3dB at cutoff, +12dB/oct rise below
- [ ] SVF BP at 1kHz: peak at cutoff, symmetric rolloff
- [ ] Resonance at 0.9: 12dB peak at cutoff (± 2dB)
- [ ] Filter stability: no self-oscillation blowup at resonance = 1.0 (output bounded)

**Testing Milestone — Integration:**
- [ ] Full voice: oscillator -> filter -> envelope -> pan produces expected output
- [ ] Sub-oscillator tracks lowest active voice frequency (verified at multiple pitches)
- [ ] 8 simultaneous voices don't exceed [-1, 1] output range (with proper gain staging)

**Testing Milestone — Mutation:**
- [ ] Remove PolyBLEP correction from saw: aliasing test MUST fail (energy above Nyquist/2 rises)
- [ ] Change envelope attack from linear to instant: attack ramp test MUST fail
- [ ] Offset SVF cutoff coefficient by 10%: frequency response test MUST fail
- [ ] Remove sub-oscillator octave division: tracking test MUST fail
- [ ] Mutation survival rate < 20%

**RT Safety Checkpoint:**
- [ ] All voice buffers pre-allocated for max polyphony (8 voices) in `prepareToPlay()`
- [ ] No `new`/`delete` in voice note-on/note-off
- [ ] SmoothedValue on all continuous params (cutoff, resonance, level, pan)
- [ ] Filter coefficient computation bounded (no iterative convergence)
- [ ] **Baseline established:** single voice < X% CPU at 44.1kHz/512

**Tag:** `v0.4.0`

---

## Phase 4.5 — Playability Fixes (`v0.4.5`) :white_check_mark:

**Goal:** Make the synth interactive and prevent grid stagnation.

- [x] BPM APVTS parameter (40-300) with rotary knob in Clock section
- [x] Auto-reseed: inject ~5 random cells after 8 stagnant generations
- [x] Virtual MIDI keyboard (`MidiKeyboardComponent`) at bottom of editor
- [x] Algorithm selector maps to GoL rule presets (Classic, HighLife, DayAndNight, Seeds, Ambient)
- [x] Per-column waveshape diversity (base shape + column offset mod 8)
- [x] MIDI note-on triggers grid reseed with note-derived RNG
- [x] Clock division and swing params read and applied per processBlock

**Testing Milestone:**
- [x] 75/75 existing tests still pass
- [ ] **Manual: BPM knob changes grid stepping speed**
- [ ] **Manual: MIDI keyboard triggers notes and reseeds grid**
- [ ] **Manual: Grid auto-reseeds after stagnation**
- [ ] **Manual: Algorithm switch changes grid behavior**

**Tag:** `v0.4.5`

---

## Phase 5 — Integration + Musicality (`v0.5.0`) :white_check_mark:

**Goal:** First playable prototype with musicality controls wired.

- [x] Quietest-voice stealing (no formal VoiceAllocator yet — done inline)
- [x] Ambient voice controls: drone sustain, note probability, velocity humanization
- [x] Musicality: melodic inertia, round-robin variation (wired into processBlock)
- [x] Wire full chain: Engine -> Quantizer -> Voices -> Stereo Mix -> Output
- [x] Density-driven dynamics: grid density modulates voice gain + filter cutoff
- [x] 7 distinct CA engines with factory pattern switching
- [x] Engine-aware grid visualization with per-engine color palettes
- [x] Adjustable grid resolution (4 sizes) with UI dropdown
- [x] 11 factory presets with corrected algorithm indices
- [ ] `VoiceAllocator` formal class (priority, hysteresis, unison) — deferred
- [ ] Portamento: continuous + stepped — deferred
- [ ] Gate time per-voice countdown — next (Phase 5.5)
- [ ] Strum spread onset delay — next (Phase 5.5)

**Testing Milestone:**
- [x] 82/82 tests pass (7 new CA engine tests)
- [ ] **Manual: All presets produce musical output** (Dark Drone under investigation)

**Tag:** `v0.5.0`

---

## Phase 5.5 — Musicality Phase 2 (`v0.5.5`)

**Goal:** Wire remaining musicality params and add engine-specific note triggering.

- [x] `gateTime`: per-voice sample countdown timer in SynthVoice (staccato at < 1.0)
- [x] `strumSpread`: onset delay per column position in SynthVoice
- [x] Engine-specific triggering: `getCellIntensity()` virtual on CellularEngine
- [x] Continuous engines (Lenia, R-D, Brownian, Swarm) modulate velocity by cell brightness
- [x] `cellActivated()` virtual for threshold-crossing detection on continuous engines
- [ ] `SmoothedValue` on all continuous parameters (filter cutoff, resonance, volume, pan, all musicality knobs) to prevent zipper noise on automation

**Testing Milestone:**
- [ ] gateTime < 1.0 produces shorter notes than step interval
- [ ] strumSpread > 0 staggers note onsets across columns
- [ ] Continuous engines produce velocity variation proportional to cell intensity
- [ ] All existing 82+ tests still pass

**Tag:** `v0.5.5`

---

## Phase 6 — Grid Persistence + Seeding (`v0.6.0`)

**Goal:** Save/load grid state with DAW project. Reproducible patterns.

- [ ] `getStateInformation()` / `setStateInformation()` with version number:
  - Version field (uint32) — first field, always
  - APVTS parameter XML
  - Grid state as base64-encoded binary blob
  - Algorithm type + seed
  - Cell age grid (compressed)
- [ ] Seed parameter: UI display (copyable), paste, "New Seed" button
- [ ] Factory pattern library (GoL patterns: Glider, Pulsar, Gosper Gun, R-Pentomino, etc.)
- [ ] CA-level freeze/drone mode: hold current grid state (stop stepping) while voices sustain, togglable via UI button and MIDI CC

**Testing Milestone — Correctness:**
- [ ] Save/load roundtrip preserves exact grid state (bit-perfect comparison)
- [ ] Save/load preserves all APVTS parameters (every param compared)
- [ ] Same seed + density produces identical grid on reload
- [ ] Factory patterns: Glider glides, Pulsar oscillates, Gosper Gun fires gliders
- [ ] State version field is written and read correctly

**Testing Milestone — Integration:**
- [ ] Save at generation N, load, continue stepping — grid matches stepping from scratch to N then continuing
- [ ] Load state from older version format: migration succeeds, no crash

**Testing Milestone — Mutation:**
- [ ] Corrupt version field: load MUST reject or migrate gracefully (not crash)
- [ ] Truncate state blob by 10 bytes: load MUST fail gracefully
- [ ] Flip one bit in grid blob: loaded grid MUST differ from saved grid
- [ ] Mutation survival rate < 20%

**RT Safety Checkpoint:**
- [ ] `getStateInformation()` runs on message thread only (asserted)
- [ ] Base64 encoding uses pre-allocated buffer (max grid size known)
- [ ] Pattern loading queued via SPSC, not applied directly from UI thread

**Tag:** `v0.6.0`

---

## Phase 7 — MIDI I/O + MIDI Learn (`v0.7.0`)

**Goal:** External keyboard control and MIDI output for driving other instruments.

- [ ] `MidiInputHandler`: key tracking, velocity -> density, channel filter, bypass
- [ ] `MidiOutputGenerator`: mirror generated notes, channel select, internal audio toggle
- [ ] `MidiLearnManager`: right-click popup, CC capture, mapping persistence
- [ ] Visual: MIDI badge on mapped controls, learn-mode pulse animation
- [ ] Chord/scale lock from MIDI: detect incoming chords and auto-adapt scale quantizer to match performer's harmonic context
- [ ] MIDI clock input: accept external MIDI clock as step source (for hardware setups — Elektron, modular, etc.)
- [ ] MIDI program change: switch factory/user presets via MIDI PC messages

**Testing Milestone — Correctness:**
- [ ] MIDI note C4 input sets quantizer root to C (verified by output frequencies)
- [ ] MIDI velocity 127 -> density 1.0, velocity 0 -> density 0.0 (linear mapping)
- [ ] Channel filter: notes on wrong channel are ignored
- [ ] MIDI output: note-on at correct pitch/velocity, note-off on voice release
- [ ] CC mapping: CC74 value 64 maps to parameter midpoint (within 1%)
- [ ] 14-bit CC pairs produce expected fine resolution

**Testing Milestone — Integration:**
- [ ] MIDI in -> key change -> quantizer -> voices: output frequencies match new key
- [ ] MIDI out -> external synth: correct notes received (loopback test)
- [ ] CC learn -> save -> load -> CC input: parameter responds correctly
- [ ] **Manual: DAW test — keyboard changes key, output drives second instrument**

**Testing Milestone — Mutation:**
- [ ] Offset MIDI note number by 1: key tracking test MUST fail
- [ ] Invert CC polarity: CC mapping test MUST fail
- [ ] Remove channel filter: channel filter test MUST fail
- [ ] Mutation survival rate < 20%

**RT Safety Checkpoint:**
- [ ] MIDI learn target is `std::atomic<int>`, no lock
- [ ] CC -> parameter mapping is array lookup (128 entries), no map/hash
- [ ] MIDI output buffer cleared and filled per-block, no accumulation

**Tag:** `v0.7.0`

---

## Phase 8 — Effects Chain (`v0.8.0`)

**Goal:** Full FX chain with reorderable slots, freeze, and safety processor.

- [ ] Integrate **Airwindows** dependency (MIT, needed for tape sat + brickwall)
- [ ] `EffectChain`: reorderable slot system with per-effect bypass and dry/wet
- [ ] Chorus (stereo width), Phaser (ping-pong), Flanger (ping-pong)
- [ ] Bitcrush (bit depth, sample rate reduction, lo-fi filter)
- [ ] Tape saturation (Airwindows-style soft clipping)
- [ ] Shimmer reverb (FDN + pitch-shifted feedback path)
- [ ] Ping-pong delay (host-synced, feedback LP, pan width)
- [ ] Master limiter
- [ ] Freeze processor (circular buffer capture, volume, LP, crossfade unfreeze)
- [ ] `SafetyProcessor` (DC filter 5Hz HP, ultrasonic LP 20kHz, brickwall -0.3dBFS)

**Testing Milestone — Correctness:**
- [ ] Chorus: output has expected LFO-modulated delay (measured via cross-correlation)
- [ ] Phaser: notch frequencies match expected all-pass cascade positions
- [ ] Flanger: comb filter peaks at expected frequencies for given delay
- [ ] Bitcrush 8-bit: output quantized to 256 levels exactly
- [ ] Bitcrush sample rate reduction: output sample-and-holds at specified rate
- [ ] Tape sat: soft clipping curve matches expected waveshaper transfer function
- [ ] Shimmer reverb: pitch-shifted feedback is +1 octave (± 5 cents, measured via FFT)
- [ ] Ping-pong delay: L/R alternation verified, delay time matches host tempo division
- [ ] Limiter: output never exceeds threshold (verified with 10 seconds of worst-case input)
- [ ] Safety DC filter: 1Hz sine reduced to < -60dB, 100Hz sine passes at > -1dB
- [ ] Safety brickwall: output never exceeds -0.3dBFS true peak (10 million samples tested)

**Testing Milestone — Integration:**
- [ ] Chain reorder produces different output (A->B->C vs C->B->A, compared)
- [ ] Bypass each effect individually: output matches chain without that effect
- [ ] Mix (dry/wet) at 0%: output matches dry signal exactly
- [ ] Freeze: captured buffer loops correctly, unfreeze fades out over 2 seconds
- [ ] Full chain + full polyphony: output stays within safety processor limits

**Testing Milestone — Mutation:**
- [ ] Remove safety processor DC filter: DC test MUST fail (1Hz passes through)
- [ ] Change brickwall threshold from -0.3dBFS to 0dBFS: true peak test MUST fail
- [ ] Remove PolyBLEP shimmer pitch shift: octave verification MUST fail
- [ ] Set chorus depth to 0: chorus output MUST equal dry (difference test fails)
- [ ] Mutation survival rate < 20%

**RT Safety Checkpoint:**
- [ ] All effect buffers pre-allocated in `prepareToPlay()`
- [ ] Chain reorder is pointer swap (UI queues new order, audio applies atomically)
- [ ] Freeze buffer is pre-allocated at max size, no runtime allocation
- [ ] SafetyProcessor: zero allocations, no branching on user input, always-on
- [ ] Full chain profiled: `processBlock` < 50% of audio budget at 44.1kHz/512

**Tag:** `v0.8.0`

---

## Phase 9 — Additional Algorithms (Batch 1) (`v0.9.0`) — MERGED INTO Phase 5

**Status:** Engines implemented in Phase 5 alongside core integration.

- [ ] `WolframCA`: 1D rule (0-255), waterfall display — deferred to future
- [x] `BriansBrain`: three-state (alive/dying/dead)
- [x] `CyclicCA`: N states, threshold, rotating spirals
- [x] `ReactionDiffusion`: Gray-Scott model, feed/kill rates, float grid
- [x] `ParticleSwarm`: agent-based, particle trails
- [ ] `AlgorithmCrossfader`: dual-engine, timed crossfade — deferred
- [x] Engine-specific visualization palettes
- [ ] NaN/Inf guards on all algorithm outputs — partial

**Testing:**
- [x] Brian's Brain: step produces activity
- [x] Cyclic CA: step advances generation
- [x] R-D: float fields and grid projection
- [x] Particle Swarm: particles deposit trails
- [x] Engine type identification via getType()

---

## Phase 10 — Lenia + Brownian (`v0.10.0`) — MERGED INTO Phase 5

**Status:** Engines implemented in Phase 5.

- [x] `LeniaEngine`: continuous-state, Gaussian kernel, growth function, bell-curve convolution
- [x] `BrownianField`: multi-walker energy deposition
- [x] Engine-aware visualization (intensity heatmaps)
- [ ] Intensity mapping: cell values -> velocity — planned for Phase 5.5

**Testing:**
- [x] Lenia: continuous state and step
- [x] Brownian: walkers deposit energy

> [!NOTE]
> Formal Orbium stability, diffusion law, and detailed correctness tests deferred to future hardening phase.

---

## Phase 11 — Arp + Vibrato (`v0.11.0`)

**Goal:** Arpeggiator and vibrato LFO for melodic variation.

- [ ] Arpeggiator: Up/Down/UpDown/Random, host-synced rate (1/4 to 1/32)
- [ ] Vibrato LFO: rate 0.1-10Hz, depth 0-100 cents
- [ ] Unison detune: all voices on one pitch, spread 0-25 cents

**Testing Milestone — Correctness:**
- [ ] Arp Up on C-E-G: produces C, E, G, C, E, G... in exact order
- [ ] Arp Down on C-E-G: produces G, E, C, G, E, C... in exact order
- [ ] Arp UpDown on C-E-G: produces C, E, G, E, C, E... (no double at endpoints)
- [ ] Arp Random: all notes appear with roughly equal probability (± 10% over 1000 steps)
- [ ] Arp host sync: at 120BPM 1/16, exactly 8 arp steps per second
- [ ] Vibrato at 5Hz, 50 cents: frequency deviation is ±50 cents sinusoidal at 5Hz (FFT verified)
- [ ] Unison 4 voices at ±7 cents: beating rate ≈ 4Hz (measured via amplitude modulation)

**Testing Milestone — Mutation:**
- [ ] Reverse Arp Up direction: sequence order test MUST fail
- [ ] Double vibrato depth coefficient: depth test MUST fail
- [ ] Mutation survival rate < 20%

**RT Safety Checkpoint:**
- [ ] Arp pattern buffer is fixed-size array (max voices)
- [ ] Vibrato LFO is pure math (sin approximation or wavetable, no allocation)

**Tag:** `v0.11.0`

---

## Phase 12 — Modulation Matrix + Stereo Drift (`v0.12.0`)

**Goal:** LFO modulation routing and spatial movement.

- [ ] 2 LFOs (sine/tri/saw/S&H, 0.01-20Hz, depth)
- [ ] Cell intensity mod source, envelope follower mod source
- [ ] Stereo drift: per-voice pan wander (Brownian/sine, rate 0-1Hz, depth 0-100%)
- [ ] 8 routing slots with source/destination/depth (-1 to +1)
- [ ] Popout window UI

**Testing Milestone — Correctness:**
- [ ] LFO sine at 1Hz: output matches sin(2*pi*t) (R² > 0.999)
- [ ] LFO triangle at 1Hz: output matches expected triangle wave
- [ ] LFO S&H: output steps at specified rate, values uniformly distributed
- [ ] LFO -> cutoff at depth 1.0: cutoff sweeps full range over one LFO cycle
- [ ] Envelope follower: tracks 200ms amplitude envelope (± 1dB)
- [ ] Stereo drift at rate 0.5Hz: pan value oscillates over 2 seconds
- [ ] Mod depth = 0: no modulation (destination unchanged)
- [ ] Mod depth = -1: modulation is inverted vs depth = +1

**Testing Milestone — Mutation:**
- [ ] Invert LFO phase: sine output test MUST show inverted waveform
- [ ] Remove depth scaling: full-range modulation always, depth=0 test MUST fail
- [ ] Mutation survival rate < 20%

**RT Safety Checkpoint:**
- [ ] Mod matrix evaluation is O(active routes), max 8 = bounded
- [ ] LFO uses wavetable or polynomial, no `sin()` per sample
- [ ] Envelope follower is one-pole filter, O(1) per sample

**Tag:** `v0.12.0`

---

## Phase 13 — OpenGL 2D + 3D Visualization (`v0.13.0`)

**Goal:** Real-time grid visualization with multiple render modes.

- [ ] `AlgoVisualizerComponent` (backend-agnostic interface for future WebGPU)
- [ ] 2D grid mode: cell colors by age gradient, playhead glow, note highlights
- [ ] 3D terrain mode: height-mapped mesh, orbit camera
- [ ] Wireframe mode: same mesh, line-rendered
- [ ] GLSL shaders (grid/terrain vertex + fragment)

**Testing Milestone:**
- [ ] Headless: GL context creation doesn't crash (graceful fallback if no GPU)
- [ ] **Manual: 2D grid cells light up on algorithm step**
- [ ] **Manual: Playhead glow moves across grid at correct tempo**
- [ ] **Manual: 3D terrain height corresponds to cell values**
- [ ] **Manual: Camera orbit is smooth, no Z-fighting or clipping**
- [ ] **Manual: Mode switching (2D/3D/Wire) is instant, no flicker**

**RT Safety Checkpoint:**
- [ ] GL rendering on its own thread, never blocks audio
- [ ] Grid data read from back buffer only (no audio thread contention)
- [ ] GL frame rate independent of audio buffer size

**Tag:** `v0.13.0`

---

## Phase 14 — UI Layout + Panels (`v0.14.0`)

**Goal:** Complete editor layout with all panels and controls.

- [ ] Algorithm selector + per-algorithm parameter panels
- [ ] Scale/key, voice panels with `NebulaDial` (gradient arc + melatonin_blur glow)
- [ ] Effect chain popout, mod matrix popout
- [ ] Grid resize + pattern presets + seed display
- [ ] Theme system: Deep Nebula, Synthwave Sunset, Void, Solar Flare
- [ ] DPI-aware, minimum 900x600, resizable with aspect ratio lock
- [ ] UI section headers: visible labels for each bottom-panel section (Clock, Tuning, Ambient, Anti-Cacophony, Humanize, Global)
- [ ] Tooltips: hover descriptions for all parameters explaining their musical effect

**Testing Milestone:**
- [ ] **Manual: All knobs/sliders respond to mouse drag**
- [ ] **Manual: Popout windows open/close, remember position**
- [ ] **Manual: Theme switch updates all colors instantly**
- [ ] **Manual: UI legible at 100%, 125%, 150% DPI**
- [ ] **Manual: Editor close/reopen restores all control states**
- [ ] **Manual: Resizing maintains layout integrity**

**RT Safety Checkpoint:**
- [ ] UI repaints don't allocate (font/path caching verified)
- [ ] Timer-driven animations don't block message thread
- [ ] Popout windows don't add audio processing overhead

**Tag:** `v0.14.0`

---

## Phase 15 — Presets + Polish (`v0.15.0`)

**Goal:** Factory presets, CPU monitoring, optimization pass.

- [ ] 18 factory presets (12 musical, 6 experimental) as BinaryData JSON
- [ ] User preset save/load, preset browser (categories, search, favorites)
- [ ] A/B preset comparison
- [ ] CPU meter in UI (processBlock time as % of audio budget)
- [ ] CPU profiling + optimization pass
- [ ] Sample rate audit: all DSP verified at 44.1k, 48k, 88.2k, 96k

**Testing Milestone:**
- [ ] All 18 factory presets load without errors
- [ ] All 18 presets produce non-silent, distinct audio output
- [ ] Preset save/load roundtrip preserves every parameter
- [ ] processBlock at 96kHz/64 samples < 70% budget
- [ ] processBlock at 44.1kHz/512 samples < 30% budget
- [ ] **Manual: User auditions all 18 presets, confirms quality**
- [ ] **Manual: CPU meter accuracy verified against external profiler**

**RT Safety Checkpoint:**
- [ ] Preset loading on message thread only, audio reads final state
- [ ] No JSON parsing on audio thread
- [ ] CPU measurement uses high-res ticks, no system calls

**Tag:** `v0.15.0`

> [!IMPORTANT]
> **Technical Debt Review #3:** Pre-release cleanup. Remove all `// TODO` and `// HACK`. Review test coverage. Check for unused code. Final abstraction review.

---

## Phase 16 — Release Candidate (`v1.0.0-rc`)

**Goal:** Full validation. Multi-DAW testing. Release build.

- [ ] Full automated test suite: 0 failures
- [ ] Mutation testing on all critical DSP: >80% kill rate
- [ ] TSAN build: no data races under stress test
- [ ] Multi-DAW testing:
  - [ ] REAPER: load, play, stop, loop, tempo change, save/load, bypass
  - [ ] Ableton Live: same battery
  - [ ] FL Studio: same battery
  - [ ] Bitwig: same battery (if available)
- [ ] 1-hour soak test: no memory growth, no CPU drift, no audio glitches
- [ ] Cold start time < 2 seconds
- [ ] Windows code signing for VST3
- [ ] Installer packaging (Inno Setup or similar)
- [ ] Release build (LTO, NDEBUG, no debug logging)
- [ ] README with screenshots, quick start guide

**RT Safety — Final Audit:**
- [ ] Zero allocations in processBlock (instrumented allocator verification)
- [ ] No mutex locks on audio thread
- [ ] No system calls on audio thread (file I/O, logging, printf)
- [ ] SafetyProcessor always active, never bypassed
- [ ] NaN/Inf guards on all algorithm outputs verified

**Tag:** `v1.0.0`

---

## Phase 17 — v1.0.0 Ships

**Deliverables:**
- 8 algorithms: GoL, Wolfram, Brian's Brain, Cyclic, R-D, Particle Swarm, Lenia, Brownian
- 8 waveshapes with PolyBLEP anti-aliasing
- 8 effects + safety processor
- 2 LFOs + mod matrix (8 routes)
- MIDI I/O + MIDI Learn
- OpenGL 2D/3D visualization
- 18 factory presets, 4 themes
- Full documentation suite

---

## v2+ Future

### v2.0 — Voice Groups + Experimental Algorithms (Tier 1)
- Voice group system: up to 4 groups, each with own algorithm/scale/grid
- Slime Mold (Physarum): pheromone trail branching networks
- Particle-Reactive Synth: audio-reactive feedback loop
- Per-algorithm 3D shaders

### v2.1 — Experimental Algorithms (Tier 2)
- Ising Model, Sand Pile, Wave Equation, Langton's Ant

### v2.2 — Synthesis Expansion + Humanization v2
- Wavetable oscillator, filter envelope, filter key tracking
- Musical gravity, octave doubling, accent patterns, articulation variation
- Custom microtuning (per-degree cent offsets)

### v3.0 — Experimental Algorithms (Tier 3)
- Markov Chain, Predator-Prey, Neural CA, DLA, L-System Grid

### v3.1 — Platform Evolution
- WebGPU renderer, MPE support, network sync, mod matrix expansion
