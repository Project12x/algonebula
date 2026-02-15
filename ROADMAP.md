# Algo Nebula — Development Roadmap

> Multi-Algorithm Generative Ambient Synthesizer
> JUCE VST3/Standalone Plugin

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

- [ ] `CellularEngine` abstract interface (`step()`, `getGrid()`, `reset()`, `seed()`)
- [ ] `GameOfLife` implementation
  - 5 rule presets: Classic (B3/S23), High Life, Day & Night, Seeds, Ambient
  - Toroidal wrapping (edges connect)
  - Cell age tracking (generations alive)
  - Seeded random initialization (`uint64_t seed`)
- [ ] Double-buffered grid swap mechanism (lock-free, audio owns front, GL reads back)
- [ ] SPSC queue for UI -> audio cell edit commands

**Testing Milestone:**
- [ ] Headless: GoL Blinker oscillates correctly (period 2)
- [ ] Headless: GoL Glider translates correctly (period 4)
- [ ] Headless: GoL still-life patterns remain stable for 1000 generations
- [ ] Headless: Seeded init produces identical grids for same seed + density
- [ ] Headless: 5 rule variants produce distinct behavior

**RT Safety Checkpoint:**
- [ ] `CellularEngine::step()` is O(rows * cols), no allocations
- [ ] Double-buffer swap is a single atomic pointer exchange
- [ ] SPSC queue drain in `processBlock()` is bounded (max N commands/block)
- [ ] Grid storage is pre-allocated at max size in `prepareToPlay()`

**Tag:** `v0.2.0`

---

## Phase 3 — Scale Quantizer + Clock + Microtuning (`v0.3.0`)

**Goal:** Musical pitch mapping and tempo-synced stepping.

- [ ] `ScaleQuantizer`: 15 scales x 12 root keys, octave range (base + span)
  - Pentatonic Major/Minor, Major/Natural Minor, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian, Whole Tone, Blues, Harmonic Minor, Melodic Minor, Chromatic
- [ ] `Microtuning`: 12-TET, Just Intonation, Pythagorean, adjustable A4 reference (420-460Hz)
- [ ] `ClockDivider`:
  - Host transport sync (play/stop/loop/tempo)
  - Free-running internal clock when host not playing
  - Playhead division (1/4, 1/8, 1/16, 1/32) and generation division (1-16 steps)
  - Swing support (50-75%)

**Testing Milestone:**
- [ ] Headless: All 15 scales x 12 keys produce correct note sets
- [ ] Headless: Microtuning frequency tables match reference values (±0.1 cent)
- [ ] Headless: Clock fires correct number of steps for given BPM + division + buffer size
- [ ] Headless: Swing timing offsets are within ±1 sample of expected

**RT Safety Checkpoint:**
- [ ] Scale lookup is array index (O(1)), no search
- [ ] Microtuning tables pre-computed in `prepareToPlay()`, not per-sample
- [ ] Clock uses integer sample counting, no floating-point drift accumulation
- [ ] No string formatting or logging in clock tick path

**Tag:** `v0.3.0`

---

## Phase 4 — PolyBLEP Synth Voices (`v0.4.0`)

**Goal:** Band-limited oscillators, envelopes, filters, sub-oscillator. Produces sound.

- [ ] `PolyBLEPOscillator`: 8 waveshapes (sine, triangle, saw, pulse, sine+oct, fifth stack, pad, bell)
- [ ] `SynthVoice`: AHDSR envelope, SVF filter (LP/HP/BP/Notch), noise layer (per-voice filtered white/pink)
- [ ] Sub-oscillator: sine, follows lowest voice, -1/-2 octave selectable, level knob
- [ ] Per-voice pan + auto-pan from grid column position
- [ ] Equal-power stereo panning law

**Testing Milestone:**
- [ ] Headless: PolyBLEP output is below -60dB above Nyquist/2 (aliasing check)
- [ ] Headless: AHDSR envelope shape matches expected curve (sample-accurate ramp checks)
- [ ] Headless: SVF filter has expected frequency response at cutoff (±1dB)
- [ ] Headless: Sub-oscillator tracks lowest voice frequency correctly
- [ ] Headless: Noise layer RMS is within expected range

**RT Safety Checkpoint:**
- [ ] All voice buffers pre-allocated for max polyphony (8 voices) in `prepareToPlay()`
- [ ] No `new`/`delete` in voice note-on/note-off
- [ ] SmoothedValue on all continuous params (cutoff, resonance, level, pan)
- [ ] Filter coefficient computation bounded (no iterative convergence)

**Tag:** `v0.4.0`

---

## Phase 5 — Integration + Humanization (`v0.5.0`)

**Goal:** First playable prototype. Engine -> Quantizer -> Voices -> Audio out.

- [ ] `VoiceAllocator`: priority (lowest/highest/latest), stealing (oldest/quietest/drop), hysteresis, unison mode
- [ ] Ambient voice controls: drone sustain, note probability, gate time, velocity humanization
- [ ] Humanization v1: swing, strum/note spread, melodic inertia, round-robin variation
- [ ] Portamento: continuous + stepped (glissando through scale degrees), legato mode
- [ ] Wire full chain: Engine -> Quantizer -> Allocator -> Voices -> Stereo Mix -> Output

**Testing Milestone:**
- [ ] Headless: Voice allocator respects max polyphony
- [ ] Headless: Stealing policy produces expected victim (oldest/quietest)
- [ ] Headless: Portamento interpolation is sample-accurate
- [ ] Headless: Drone sustain extends note beyond cell death with correct probability
- [ ] Headless: Melodic inertia biases toward small intervals (statistical test)
- [ ] **Manual: User confirms first playable prototype produces musical output**

**RT Safety Checkpoint:**
- [ ] Full audio path profiled: `processBlock` < 30% of audio budget at 44.1kHz/512 samples
- [ ] No allocations in full render chain (verify with MSVC debug heap or AddressSanitizer)
- [ ] Voice stealing is O(N) where N = max voices (8), no sorting
- [ ] All random number generation uses pre-seeded, allocation-free PRNG

**Tag:** `v0.5.0` — **First Playable Milestone**

---

## Phase 6 — Grid Persistence + Seeding (`v0.6.0`)

**Goal:** Save/load grid state with DAW project. Reproducible patterns.

- [ ] `getStateInformation()` / `setStateInformation()`:
  - APVTS parameter XML
  - Grid state as base64-encoded binary blob
  - Algorithm type + seed
  - Cell age grid (compressed)
- [ ] Seed parameter: UI display (copyable), paste, "New Seed" button
- [ ] Factory pattern library (GoL patterns: Glider, Pulsar, Gosper Gun, R-Pentomino, etc.)

**Testing Milestone:**
- [ ] Headless: Save/load roundtrip preserves exact grid state (bit-perfect)
- [ ] Headless: Save/load preserves all APVTS parameters
- [ ] Headless: Same seed + density produces identical grid on reload
- [ ] Headless: Factory patterns load correctly and are stable/oscillating as expected

**RT Safety Checkpoint:**
- [ ] `getStateInformation()` runs on message thread only (asserted)
- [ ] Base64 encoding uses pre-allocated buffer (max grid size known)
- [ ] Pattern loading queued via SPSC, not applied directly from UI thread

**Tag:** `v0.6.0`

---

## Phase 7 — MIDI I/O + MIDI Learn (`v0.7.0`)

**Goal:** External keyboard control and MIDI output for driving other instruments.

- [ ] `MidiInputHandler`: key tracking, velocity -> density, channel filter, bypass toggle
- [ ] `MidiOutputGenerator`: mirror generated notes to MIDI output, channel select, internal audio toggle
- [ ] `MidiLearnManager`: right-click popup, CC capture, mapping persistence in state
- [ ] Visual: MIDI badge on mapped controls, learn-mode pulse animation

**Testing Milestone:**
- [ ] Headless: MIDI note-in changes quantizer root correctly
- [ ] Headless: MIDI output buffer contains expected note-on/off messages
- [ ] Headless: CC mapping roundtrip (learn -> save -> load -> verify)
- [ ] **Manual: DAW test — keyboard changes key, output drives second instrument**

**RT Safety Checkpoint:**
- [ ] MIDI learn target is `std::atomic<int>`, no lock
- [ ] CC -> parameter mapping is array lookup (128 entries), no map/hash
- [ ] MIDI output buffer cleared and filled per-block, no accumulation

**Tag:** `v0.7.0`

---

## Phase 8 — Effects Chain (`v0.8.0`)

**Goal:** Full FX chain with reorderable slots, freeze, and safety processor.

- [ ] `EffectChain`: reorderable slot system with per-effect bypass and dry/wet
- [ ] Chorus (stereo width), Phaser (ping-pong), Flanger (ping-pong)
- [ ] Bitcrush (bit depth, sample rate reduction, lo-fi filter)
- [ ] Tape saturation (Airwindows-style soft clipping)
- [ ] Shimmer reverb (FDN + pitch-shifted feedback path)
- [ ] Ping-pong delay (host-synced, feedback LP, pan width)
- [ ] Master limiter
- [ ] Freeze processor (circular buffer capture, volume, LP filter, crossfade unfreeze)
- [ ] `SafetyProcessor` (DC offset filter 5Hz HP, ultrasonic LP 20kHz, brickwall at -0.3dBFS)

**Testing Milestone:**
- [ ] Headless: Each effect produces non-silent, non-identical output vs bypass
- [ ] Headless: Effect chain reordering produces different output (order matters)
- [ ] Headless: Safety processor clamps output to -0.3dBFS true peak
- [ ] Headless: DC offset filter removes 1Hz test tone to < -60dB
- [ ] Headless: Freeze capture + playback matches source (crossfade verification)
- [ ] Mutation tests: bypass flag, mix param, chain order — verify tests catch breakage

**RT Safety Checkpoint:**
- [ ] All effect buffers pre-allocated in `prepareToPlay()`
- [ ] Chain reorder is pointer swap (UI queues new order, audio applies atomically)
- [ ] Freeze buffer is pre-allocated at max size, no runtime allocation
- [ ] SafetyProcessor: zero allocations, no branching on user input, always-on
- [ ] Full chain profiled: `processBlock` < 50% of audio budget at 44.1kHz/512

**Tag:** `v0.8.0`

---

## Phase 9 — Additional Algorithms (Batch 1) (`v0.9.0`)

**Goal:** Five more algorithms: Wolfram, Brian's Brain, Cyclic, Reaction-Diffusion, Particle Swarm.

- [ ] `WolframCA`: 1D rule (0-255), waterfall display, rule presets
- [ ] `BriansBrain`: three-state (alive/dying/dead), constant sparking
- [ ] `CyclicCA`: N states (3-16), threshold, rotating spirals
- [ ] `ReactionDiffusion`: Gray-Scott model, feed/kill rates, floating-point grid
- [ ] `ParticleSwarm`: agent-based, count/speed/cohesion params, flocking
- [ ] `AlgorithmCrossfader`: dual-engine simultaneous run, timed crossfade, no silence gap
- [ ] Factory patterns for each algorithm

**Testing Milestone:**
- [ ] Headless: Each algorithm produces distinct output from GoL (statistical comparison)
- [ ] Headless: Wolfram Rule 110 produces known Turing-complete pattern
- [ ] Headless: Brian's Brain maintains constant activity (never fully dies)
- [ ] Headless: Cyclic CA produces rotation (directional movement detected)
- [ ] Headless: R-D produces stable Turing patterns at known feed/kill values
- [ ] Headless: Particle swarm maintains group cohesion (centroid tracking)
- [ ] Headless: Crossfade produces no silence gap during algorithm switch
- [ ] Headless: Crossfade output is click-free (zero-crossing analysis)

**RT Safety Checkpoint:**
- [ ] R-D uses pre-allocated float grid, no per-step allocation
- [ ] Particle swarm is O(N) where N = particle count (max 32)
- [ ] Crossfader pre-allocates both engines, no construction during crossfade
- [ ] All new algorithms profiled: `step()` < 5% of audio budget

**Tag:** `v0.9.0`

---

## Phase 10 — Lenia + Brownian (`v0.10.0`)

**Goal:** Continuous-state algorithms for organic, ambient textures.

- [ ] `Lenia`: continuous-state GoL, Gaussian neighborhood kernel, growth function, configurable radius/center/width/timestep
- [ ] `BrownianField`: multi-walker (1-16) random walk, correlation, drift bias, step size
- [ ] Factory patterns for both
- [ ] Intensity mapping: Lenia cell values (0.0-1.0) -> velocity, Brownian walker position -> pitch

**Testing Milestone:**
- [ ] Headless: Lenia Orbium creature remains stable for 1000 steps
- [ ] Headless: Lenia total cell intensity is bounded (no runaway growth)
- [ ] Headless: Brownian walker distribution is statistically uniform over long runs
- [ ] Headless: Brownian correlation parameter produces expected variance reduction
- [ ] Headless: Intensity mapping produces velocity range [0, 127]

**RT Safety Checkpoint:**
- [ ] Lenia kernel pre-computed in `prepareToPlay()`, not per-step
- [ ] Lenia uses pre-allocated float grids (two: current + next)
- [ ] Brownian PRNG is allocation-free, deterministic with seed

**Tag:** `v0.10.0`

---

## Phase 11 — Arp + Vibrato (`v0.11.0`)

**Goal:** Arpeggiator and vibrato LFO for melodic variation.

- [ ] Arpeggiator: Up/Down/UpDown/Random, host-synced rate (1/4 to 1/32)
- [ ] Vibrato LFO: rate 0.1-10Hz, depth 0-100 cents
- [ ] Unison detune: all voices on one pitch, spread 0-25 cents

**Testing Milestone:**
- [ ] Headless: Arp Up pattern on C major triad produces correct sequence
- [ ] Headless: Vibrato depth matches expected frequency deviation (±1 cent)
- [ ] Headless: Unison detune spread matches expected cent offsets

**RT Safety Checkpoint:**
- [ ] Arp pattern buffer is fixed-size array (max voices)
- [ ] Vibrato LFO is pure math (no table lookup allocation)

**Tag:** `v0.11.0`

---

## Phase 12 — Modulation Matrix + Stereo Drift (`v0.12.0`)

**Goal:** LFO modulation routing and spatial movement.

- [ ] 2 LFOs (sine/tri/saw/S&H, 0.01-20Hz, depth)
- [ ] Cell intensity mod source (age/neighbor count, 0-1)
- [ ] Envelope follower mod source
- [ ] Stereo drift: per-voice pan wander (Brownian/sine LFO, rate 0-1Hz, depth 0-100%)
- [ ] 8 routing slots, each with source/destination/depth (-1 to +1)
- [ ] Popout window UI

**Testing Milestone:**
- [ ] Headless: LFO -> cutoff modulation produces expected frequency sweep
- [ ] Headless: Envelope follower tracks amplitude correctly (±1dB)
- [ ] Headless: Stereo drift produces per-voice pan movement over time
- [ ] Headless: Mod routing depth = 0 produces no modulation

**RT Safety Checkpoint:**
- [ ] Mod matrix evaluation is O(active routes), max 8 = bounded
- [ ] LFO phase is per-sample increment, no trig function per sample (use wavetable or polynomial)
- [ ] Envelope follower is one-pole filter, O(1) per sample

**Tag:** `v0.12.0`

---

## Phase 13 — OpenGL 2D + 3D Visualization (`v0.13.0`)

**Goal:** Real-time grid visualization with multiple render modes.

- [ ] `AlgoVisualizerComponent` (backend-agnostic interface for future WebGPU swap)
- [ ] 2D grid mode: color cells by age gradient, playhead glow stripe, note highlight
- [ ] 3D terrain mode: height-mapped mesh from grid, orbit camera
- [ ] Wireframe mode: same mesh, line-rendered
- [ ] GLSL shaders (grid_vertex/fragment, terrain_vertex/fragment)
- [ ] GL thread reads double-buffered back copy (no lock, 30-60fps)

**Testing Milestone:**
- [ ] **Manual: 2D grid renders correctly, cells light up on step**
- [ ] **Manual: 3D terrain height corresponds to cell values**
- [ ] **Manual: Camera orbit is smooth, no Z-fighting**
- [ ] Headless: GL context creation doesn't crash in headless (graceful fallback)

**RT Safety Checkpoint:**
- [ ] GL rendering is on its own thread, never blocks audio
- [ ] Grid data read from back buffer only (no contention with audio front buffer)
- [ ] GL thread frame rate is independent of audio buffer size

**Tag:** `v0.13.0`

---

## Phase 14 — UI Layout + Panels (`v0.14.0`)

**Goal:** Complete editor layout with all panels and controls.

- [ ] Algorithm selector + per-algorithm parameter panels
- [ ] Scale/key selector, voice panels with all knob controls
- [ ] `NebulaDial` with gradient arc + glow (melatonin_blur)
- [ ] Effect chain popout window (reorderable slots, bypass, mix)
- [ ] Mod matrix popout window (routing grid)
- [ ] Grid resize controls + pattern preset dropdown + seed display
- [ ] Theme system: Deep Nebula (default), Synthwave Sunset, Void, Solar Flare
- [ ] DPI-aware layout, minimum 900x600, resizable with aspect ratio lock

**Testing Milestone:**
- [ ] **Manual: All controls respond to mouse interaction**
- [ ] **Manual: Popout windows open/close correctly**
- [ ] **Manual: Theme switching updates all colors instantly**
- [ ] **Manual: UI is legible at 100%, 125%, 150% DPI scaling**
- [ ] **Manual: Editor close/reopen restores all control states**

**RT Safety Checkpoint:**
- [ ] UI repaints don't allocate (font caching, path caching verified)
- [ ] Timer-driven animations don't block the message thread
- [ ] Popout windows don't create additional audio processing overhead

**Tag:** `v0.14.0`

---

## Phase 15 — Presets + Polish (`v0.15.0`)

**Goal:** Factory presets, CPU monitoring, optimization pass.

- [ ] 18 factory presets (12 musical, 6 experimental) embedded as BinaryData JSON
- [ ] User preset save/load (JSON in plugin data directory)
- [ ] Preset browser: category filter, search, favorites
- [ ] A/B preset comparison
- [ ] CPU meter in UI (processBlock time as % of audio budget)
- [ ] CPU profiling + optimization pass
- [ ] Sample rate handling audit (verify all DSP at 44.1k, 48k, 88.2k, 96k)
- [ ] All documentation updated

**Testing Milestone:**
- [ ] Headless: All 18 factory presets load without errors
- [ ] Headless: Preset save/load roundtrip preserves all parameters
- [ ] Headless: processBlock at 96kHz/64 samples stays < 70% budget
- [ ] **Manual: User auditions all 18 presets, confirms quality**
- [ ] **Manual: CPU meter reads correctly under load**

**RT Safety Checkpoint:**
- [ ] Preset loading queued to message thread, audio thread only reads final state
- [ ] No JSON parsing on audio thread
- [ ] CPU measurement uses `juce::Time::getHighResolutionTicks()`, no system calls

**Tag:** `v0.15.0`

---

## Phase 16 — Release Candidate (`v1.0.0-rc`)

**Goal:** Full validation pass. Multi-DAW testing. Release build.

- [ ] Full automated test suite passes (0 failures)
- [ ] Mutation testing: >80% mutation kill rate on critical DSP code
- [ ] TSAN build: no data races detected under stress test
- [ ] Multi-DAW testing:
  - [ ] REAPER: load, play, stop, loop, tempo change, save/load project, bypass
  - [ ] Ableton Live: same battery
  - [ ] FL Studio: same battery
  - [ ] Bitwig: same battery (if available)
- [ ] Installer packaging (Inno Setup or similar)
- [ ] Release build optimizations (LTO, NDEBUG, no debug logging)
- [ ] README with screenshots, quick start guide

**Testing Milestone:**
- [ ] All automated tests pass
- [ ] All manual verification items from Phases 5-15 re-checked
- [ ] 1-hour soak test: no memory growth, no CPU drift, no audio glitches
- [ ] Cold start time < 2 seconds

**RT Safety Checkpoint:**
- [ ] Final RT audit: zero allocations in processBlock (verified with instrumented allocator)
- [ ] No mutex locks on audio thread
- [ ] No system calls on audio thread (file I/O, logging, etc.)
- [ ] SafetyProcessor never bypassed in release build

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
- 18 factory presets
- 4 themes
- Full documentation

---

## v2+ Future

### v2.0 — Voice Groups + Experimental Algorithms (Tier 1)
- Voice group system: up to 4 groups, each with own algorithm/scale/grid
- Slime Mold (Physarum): pheromone trail branching networks
- Particle-Reactive Synth: particles respond to audio output (feedback loop)
- Per-algorithm 3D shaders

### v2.1 — Experimental Algorithms (Tier 2)
- Ising Model, Sand Pile, Wave Equation, Langton's Ant

### v2.2 — Synthesis Expansion + Humanization v2
- Wavetable oscillator, filter envelope, filter key tracking
- Musical gravity, octave doubling, accent patterns, articulation variation
- Custom microtuning (per-degree cent offsets)

### v3.0 — Experimental Algorithms (Tier 3)
- Markov Chain Grid, Predator-Prey, Neural CA, DLA, L-System Grid

### v3.1 — Platform Evolution
- WebGPU renderer (replace OpenGL)
- MPE support, network sync
- Mod matrix expansion (16+ routes)
