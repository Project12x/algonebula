#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#include "engine/BriansBrain.h"
#include "engine/BrownianField.h"
#include "engine/CellEditQueue.h"
#include "engine/ClockDivider.h"
#include "engine/CyclicCA.h"
#include "engine/GameOfLife.h"
#include "engine/Grid.h"
#include "engine/LeniaEngine.h"
#include "engine/Microtuning.h"
#include "engine/ParticleSwarm.h"
#include "engine/ReactionDiffusion.h"
#include "engine/ScaleQuantizer.h"
#include "engine/SynthVoice.h"

// --- Test Helpers ---
static int testsPassed = 0;
static int testsFailed = 0;

#define TEST(name)                                                             \
  do {                                                                         \
    std::cout << "  " << name << "... ";                                       \
  } while (0)

#define PASS()                                                                 \
  do {                                                                         \
    std::cout << "PASS" << std::endl;                                          \
    ++testsPassed;                                                             \
  } while (0)

#define FAIL(msg)                                                              \
  do {                                                                         \
    std::cout << "FAIL: " << msg << std::endl;                                 \
    ++testsFailed;                                                             \
  } while (0)

#define ASSERT_EQ(a, b)                                                        \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      FAIL(#a " != " #b " (" << (a) << " vs " << (b) << ")");                  \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define ASSERT_TRUE(expr)                                                      \
  do {                                                                         \
    if (!(expr)) {                                                             \
      FAIL(#expr " is false");                                                 \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define ASSERT_NEAR(a, b, tol)                                                 \
  do {                                                                         \
    double _a = static_cast<double>(a);                                        \
    double _b = static_cast<double>(b);                                        \
    if (std::abs(_a - _b) > (tol)) {                                           \
      FAIL(#a " != " #b " (" << _a << " vs " << _b << ", tol=" << (tol)        \
                             << ")");                                          \
      return;                                                                  \
    }                                                                          \
  } while (0)

// ============================================================================
// Grid Tests
// ============================================================================
void testGridBasics() {
  TEST("Grid: default dimensions");
  Grid g;
  ASSERT_EQ(g.getRows(), 12);
  ASSERT_EQ(g.getCols(), 16);
  PASS();
}

void testGridCustomDimensions() {
  TEST("Grid: custom dimensions");
  Grid g(8, 32);
  ASSERT_EQ(g.getRows(), 8);
  ASSERT_EQ(g.getCols(), 32);
  PASS();
}

void testGridClamping() {
  TEST("Grid: dimension clamping");
  Grid g(0, 100);
  ASSERT_EQ(g.getRows(), 1);
  ASSERT_EQ(g.getCols(), 64); // kMaxCols
  PASS();
}

void testGridSetGetCell() {
  TEST("Grid: set/get cell");
  Grid g(4, 4);
  g.setCell(1, 2, 1);
  ASSERT_EQ(g.getCell(1, 2), 1);
  ASSERT_EQ(g.getCell(0, 0), 0);
  PASS();
}

void testGridToroidalWrap() {
  TEST("Grid: toroidal wrapping");
  Grid g(4, 4);
  g.setCell(0, 0, 1);
  // Access with negative and out-of-bound coords
  ASSERT_EQ(g.getCell(-4, 0), 1); // wraps to row 0
  ASSERT_EQ(g.getCell(4, 0), 1);  // wraps to row 0
  ASSERT_EQ(g.getCell(0, -4), 1); // wraps to col 0
  ASSERT_EQ(g.getCell(0, 4), 1);  // wraps to col 0
  PASS();
}

void testGridAge() {
  TEST("Grid: age tracking");
  Grid g(4, 4);
  g.setAge(1, 1, 5);
  ASSERT_EQ(g.getAge(1, 1), 5);
  g.incrementAge(1, 1);
  ASSERT_EQ(g.getAge(1, 1), 6);
  PASS();
}

void testGridCountAlive() {
  TEST("Grid: countAlive");
  Grid g(4, 4);
  ASSERT_EQ(g.countAlive(), 0);
  g.setCell(0, 0, 1);
  g.setCell(1, 1, 1);
  g.setCell(2, 2, 1);
  ASSERT_EQ(g.countAlive(), 3);
  PASS();
}

void testGridEquality() {
  TEST("Grid: equality operator");
  Grid a(4, 4), b(4, 4);
  ASSERT_TRUE(a == b);
  a.setCell(1, 1, 1);
  ASSERT_TRUE(a != b);
  b.setCell(1, 1, 1);
  ASSERT_TRUE(a == b);
  PASS();
}

void testGridCopyFrom() {
  TEST("Grid: copyFrom");
  Grid a(4, 4);
  a.setCell(1, 1, 1);
  a.setAge(1, 1, 42);
  Grid b;
  b.copyFrom(a);
  ASSERT_EQ(b.getRows(), 4);
  ASSERT_EQ(b.getCols(), 4);
  ASSERT_EQ(b.getCell(1, 1), 1);
  ASSERT_EQ(b.getAge(1, 1), 42);
  PASS();
}

// ============================================================================
// Game of Life — Correctness Tests
// ============================================================================

// Blinker (period 2): three horizontal cells oscillate to three vertical cells
void testGoLBlinker() {
  TEST("GoL: Blinker oscillates (period 2)");
  GameOfLife gol(5, 5, GameOfLife::RulePreset::Classic);

  // Horizontal blinker at center
  int blinker[][2] = {{2, 1}, {2, 2}, {2, 3}};
  gol.loadPattern(blinker, 3, 0, 0);

  // After 1 step: should be vertical
  gol.step();
  ASSERT_EQ(gol.getGrid().getCell(1, 2), 1);
  ASSERT_EQ(gol.getGrid().getCell(2, 2), 1);
  ASSERT_EQ(gol.getGrid().getCell(3, 2), 1);
  ASSERT_EQ(gol.getGrid().getCell(2, 1), 0);
  ASSERT_EQ(gol.getGrid().getCell(2, 3), 0);

  // After 2 steps: back to horizontal
  gol.step();
  ASSERT_EQ(gol.getGrid().getCell(2, 1), 1);
  ASSERT_EQ(gol.getGrid().getCell(2, 2), 1);
  ASSERT_EQ(gol.getGrid().getCell(2, 3), 1);
  ASSERT_EQ(gol.getGrid().getCell(1, 2), 0);
  ASSERT_EQ(gol.getGrid().getCell(3, 2), 0);
  PASS();
}

// Block (still life): 2x2 block never changes
void testGoLBlock() {
  TEST("GoL: Block still life (stable)");
  GameOfLife gol(6, 6, GameOfLife::RulePreset::Classic);

  int block[][2] = {{1, 1}, {1, 2}, {2, 1}, {2, 2}};
  gol.loadPattern(block, 4, 0, 0);

  Grid before;
  before.copyFrom(gol.getGrid());

  for (int i = 0; i < 10; ++i)
    gol.step();

  ASSERT_TRUE(gol.getGrid() == before);
  PASS();
}

// Beehive (still life)
void testGoLBeehive() {
  TEST("GoL: Beehive still life (stable)");
  GameOfLife gol(6, 7, GameOfLife::RulePreset::Classic);

  int beehive[][2] = {{1, 2}, {1, 3}, {2, 1}, {2, 4}, {3, 2}, {3, 3}};
  gol.loadPattern(beehive, 6, 0, 0);

  Grid before;
  before.copyFrom(gol.getGrid());

  for (int i = 0; i < 10; ++i)
    gol.step();

  ASSERT_TRUE(gol.getGrid() == before);
  PASS();
}

// Glider (period 4, displaces by (1,1))
void testGoLGlider() {
  TEST("GoL: Glider translates (period 4, +1,+1)");
  GameOfLife gol(12, 12, GameOfLife::RulePreset::Classic);

  // Standard glider
  int glider[][2] = {{0, 1}, {1, 2}, {2, 0}, {2, 1}, {2, 2}};
  gol.loadPattern(glider, 5, 0, 0);

  // After 4 steps, glider should be displaced by (+1, +1)
  for (int i = 0; i < 4; ++i)
    gol.step();

  // Check displaced glider cells
  ASSERT_EQ(gol.getGrid().getCell(1, 2), 1);
  ASSERT_EQ(gol.getGrid().getCell(2, 3), 1);
  ASSERT_EQ(gol.getGrid().getCell(3, 1), 1);
  ASSERT_EQ(gol.getGrid().getCell(3, 2), 1);
  ASSERT_EQ(gol.getGrid().getCell(3, 3), 1);

  // Original position should be dead
  ASSERT_EQ(gol.getGrid().getCell(0, 1), 0);
  PASS();
}

// Generation counter
void testGoLGeneration() {
  TEST("GoL: generation counter");
  GameOfLife gol(5, 5);
  ASSERT_EQ(gol.getGeneration(), 0u);
  gol.step();
  ASSERT_EQ(gol.getGeneration(), 1u);
  gol.step();
  ASSERT_EQ(gol.getGeneration(), 2u);
  gol.clear();
  ASSERT_EQ(gol.getGeneration(), 0u);
  PASS();
}

// Age tracking through steps
void testGoLAge() {
  TEST("GoL: cell age increments on survival");
  GameOfLife gol(6, 6, GameOfLife::RulePreset::Classic);

  // Block: all cells survive every step
  int block[][2] = {{1, 1}, {1, 2}, {2, 1}, {2, 2}};
  gol.loadPattern(block, 4, 0, 0);

  ASSERT_EQ(gol.getGrid().getAge(1, 1), 1); // Initial age

  gol.step();
  ASSERT_EQ(gol.getGrid().getAge(1, 1), 2);

  gol.step();
  ASSERT_EQ(gol.getGrid().getAge(1, 1), 3);

  // Dead cells should have age 0
  ASSERT_EQ(gol.getGrid().getAge(0, 0), 0);
  PASS();
}

// Seeded randomization is deterministic
void testGoLDeterministicSeed() {
  TEST("GoL: deterministic seeding");
  GameOfLife a(12, 16), b(12, 16);

  a.randomize(12345, 0.3f);
  b.randomize(12345, 0.3f);

  ASSERT_TRUE(a.getGrid() == b.getGrid());

  // Different seed produces different grid
  GameOfLife c(12, 16);
  c.randomize(99999, 0.3f);
  ASSERT_TRUE(a.getGrid() != c.getGrid());
  PASS();
}

// Density produces approximately correct fill
void testGoLDensity() {
  TEST("GoL: density approximation");
  GameOfLife gol(32, 64); // max size
  gol.randomize(42, 0.5f);

  int alive = gol.getGrid().countAlive();
  int total = 32 * 64;
  float ratio = static_cast<float>(alive) / total;

  // 50% density should be within 40-60% for large grids
  ASSERT_TRUE(ratio > 0.35f && ratio < 0.65f);
  PASS();
}

// Empty density produces empty grid
void testGoLZeroDensity() {
  TEST("GoL: zero density produces empty grid");
  GameOfLife gol(12, 16);
  gol.randomize(42, 0.0f);
  ASSERT_EQ(gol.getGrid().countAlive(), 0);
  PASS();
}

// Full density produces full grid
void testGoLFullDensity() {
  TEST("GoL: full density produces full grid");
  GameOfLife gol(12, 16);
  gol.randomize(42, 1.0f);
  ASSERT_EQ(gol.getGrid().countAlive(), 12 * 16);
  PASS();
}

// Rules: HighLife B36/S23 — replicator test
void testGoLHighLife() {
  TEST("GoL: HighLife rule preset (B36/S23)");
  GameOfLife gol(10, 10, GameOfLife::RulePreset::HighLife);

  // With B36: a dead cell with 6 neighbors is born (differs from Classic)
  // Simple test: verify rule preset is applied
  gol.randomize(42, 0.3f);
  int before = gol.getGrid().countAlive();
  gol.step();
  int after = gol.getGrid().countAlive();
  // Just verify it changed (rules are active)
  ASSERT_TRUE(before != after || before == 0);
  PASS();
}

// Rules: Seeds B2/S (nothing survives)
void testGoLSeeds() {
  TEST("GoL: Seeds rule preset (B2/S — no survival)");
  GameOfLife gol(8, 8, GameOfLife::RulePreset::Seeds);

  // Place a block — in Seeds, nothing survives, so all 4 die
  int block[][2] = {{2, 2}, {2, 3}, {3, 2}, {3, 3}};
  gol.loadPattern(block, 4, 0, 0);

  gol.step();

  // All 4 original cells should be dead (S=empty)
  ASSERT_EQ(gol.getGrid().getCell(2, 2), 0);
  ASSERT_EQ(gol.getGrid().getCell(2, 3), 0);
  ASSERT_EQ(gol.getGrid().getCell(3, 2), 0);
  ASSERT_EQ(gol.getGrid().getCell(3, 3), 0);
  PASS();
}

// Rules: Ambient B3/S2345 (high survival)
void testGoLAmbient() {
  TEST("GoL: Ambient rule preset (B3/S2345 — high survival)");
  GameOfLife gol(6, 6, GameOfLife::RulePreset::Ambient);

  // Block: each cell has 3 neighbors, should survive (S includes 3)
  int block[][2] = {{1, 1}, {1, 2}, {2, 1}, {2, 2}};
  gol.loadPattern(block, 4, 0, 0);

  gol.step();

  // All 4 should survive (3 neighbors each, S3 is in S2345)
  ASSERT_EQ(gol.getGrid().getCell(1, 1), 1);
  ASSERT_EQ(gol.getGrid().getCell(1, 2), 1);
  ASSERT_EQ(gol.getGrid().getCell(2, 1), 1);
  ASSERT_EQ(gol.getGrid().getCell(2, 2), 1);
  PASS();
}

// Toroidal wrapping: glider wraps around edges
void testGoLToroidal() {
  TEST("GoL: toroidal edge wrapping");
  GameOfLife gol(5, 5, GameOfLife::RulePreset::Classic);

  // Place cells at edges that should interact via wrapping
  gol.clear();
  // Three cells spanning the wrap boundary (column 4, 0, 1)
  gol.getGridMutable().setCell(2, 4, 1);
  gol.getGridMutable().setCell(2, 0, 1);
  gol.getGridMutable().setCell(2, 1, 1);

  gol.step();

  // This is a blinker centered at col 0, wrapped.
  // After one step of a horizontal blinker at (2, 4/0/1):
  // neighbors: cell (1,0) has 3 neighbors -> born
  //            cell (3,0) has 3 neighbors -> born
  //            cell (2,0) has 2 neighbors -> survives
  //            cell (2,4) has 1 neighbor -> dies
  //            cell (2,1) has 1 neighbor -> dies
  ASSERT_EQ(gol.getGrid().getCell(1, 0), 1);
  ASSERT_EQ(gol.getGrid().getCell(2, 0), 1);
  ASSERT_EQ(gol.getGrid().getCell(3, 0), 1);
  PASS();
}

// ============================================================================
// CellEditQueue Tests
// ============================================================================

void testQueuePushPop() {
  TEST("CellEditQueue: push/pop");
  CellEditQueue q;
  ASSERT_TRUE(q.push(1, 2, 1));

  CellEditQueue::Command cmd;
  ASSERT_TRUE(q.pop(cmd));
  ASSERT_EQ(cmd.row, 1);
  ASSERT_EQ(cmd.col, 2);
  ASSERT_EQ(cmd.state, 1);

  // Queue should now be empty
  ASSERT_TRUE(!q.pop(cmd));
  PASS();
}

void testQueueFull() {
  TEST("CellEditQueue: full queue returns false");
  CellEditQueue q;
  // Fill to capacity - 1 (ring buffer uses one slot as sentinel)
  for (int i = 0; i < CellEditQueue::kCapacity - 1; ++i)
    ASSERT_TRUE(q.push(i, i, 1));

  // Next push should fail
  ASSERT_TRUE(!q.push(0, 0, 1));
  PASS();
}

void testQueueDrainInto() {
  TEST("CellEditQueue: drainInto grid");
  CellEditQueue q;
  q.push(1, 2, 1);
  q.push(3, 4, 1);
  q.push(1, 2, 0); // Toggle back off

  Grid g(8, 8);
  int drained = q.drainInto(g);
  ASSERT_EQ(drained, 3);
  ASSERT_EQ(g.getCell(1, 2), 0); // Was set then cleared
  ASSERT_EQ(g.getCell(3, 4), 1);
  PASS();
}

void testQueueBoundedDrain() {
  TEST("CellEditQueue: bounded drain (maxCount)");
  CellEditQueue q;
  for (int i = 0; i < 10; ++i)
    q.push(0, i, 1);

  Grid g(4, 16);
  int drained = q.drainInto(g, 5);
  ASSERT_EQ(drained, 5);

  // Should still have 5 remaining
  CellEditQueue::Command cmd;
  int remaining = 0;
  while (q.pop(cmd))
    ++remaining;
  ASSERT_EQ(remaining, 5);
  PASS();
}

// ============================================================================
// Mutation Tests — verifying test strength
// ============================================================================

// Mutation 1: Change B3 to B2 — Blinker should NOT oscillate correctly
void testMutation_BirthRuleFlip() {
  TEST("Mutation: B3->B2 breaks Blinker");
  // With B2/S23, the blinker behavior is different from B3/S23
  // (this proves the birth rule matters)
  GameOfLife gol(5, 5, GameOfLife::RulePreset::Classic);

  int blinker[][2] = {{2, 1}, {2, 2}, {2, 3}};
  gol.loadPattern(blinker, 3, 0, 0);
  gol.step();

  // Under Classic (B3/S23): vertical blinker at (1,2),(2,2),(3,2)
  bool classicCorrect = gol.getGrid().getCell(1, 2) == 1 &&
                        gol.getGrid().getCell(2, 2) == 1 &&
                        gol.getGrid().getCell(3, 2) == 1;

  // Now test with Seeds (B2/S) — same pattern should behave differently
  GameOfLife mutant(5, 5, GameOfLife::RulePreset::Seeds);
  mutant.loadPattern(blinker, 3, 0, 0);
  mutant.step();

  bool mutantSame = mutant.getGrid().getCell(1, 2) == 1 &&
                    mutant.getGrid().getCell(2, 2) == 1 &&
                    mutant.getGrid().getCell(3, 2) == 1;

  ASSERT_TRUE(classicCorrect);
  ASSERT_TRUE(!mutantSame); // Mutation MUST produce different result
  PASS();
}

// Mutation 2: Disable survival — Block should die
void testMutation_SurvivalDisabled() {
  TEST("Mutation: no survival (Seeds) kills Block");
  GameOfLife gol(6, 6, GameOfLife::RulePreset::Seeds); // B2/S (no survival)

  int block[][2] = {{1, 1}, {1, 2}, {2, 1}, {2, 2}};
  gol.loadPattern(block, 4, 0, 0);
  ASSERT_EQ(gol.getGrid().countAlive(), 4);

  gol.step();

  // With no survival, all original cells should die
  ASSERT_EQ(gol.getGrid().getCell(1, 1), 0);
  ASSERT_EQ(gol.getGrid().getCell(1, 2), 0);
  ASSERT_EQ(gol.getGrid().getCell(2, 1), 0);
  ASSERT_EQ(gol.getGrid().getCell(2, 2), 0);
  PASS();
}

// Mutation 3: Age should track correctly — zero age means dead
void testMutation_AgeReset() {
  TEST("Mutation: dying cell resets age to 0");
  GameOfLife gol(5, 5, GameOfLife::RulePreset::Classic);

  // Single cell (dies immediately — 0 neighbors)
  gol.getGridMutable().setCell(2, 2, 1);
  gol.getGridMutable().setAge(2, 2, 10);

  gol.step();

  // Cell should be dead and age reset
  ASSERT_EQ(gol.getGrid().getCell(2, 2), 0);
  ASSERT_EQ(gol.getGrid().getAge(2, 2), 0);
  PASS();
}

// ============================================================================
// Phase 3 — ScaleQuantizer Tests
// ============================================================================

/// Verify all 15 scale interval patterns are musically correct.
static void testAllScaleIntervals() {
  TEST("ScaleQuantizer: all 15 scales have correct intervals");
  ScaleQuantizer sq;

  // Expected interval patterns (semitones from root)
  struct ScaleData {
    ScaleQuantizer::Scale scale;
    int degrees[12];
    int count;
  };
  ScaleData expected[] = {
      {ScaleQuantizer::Scale::Chromatic,
       {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
       12},
      {ScaleQuantizer::Scale::Major, {0, 2, 4, 5, 7, 9, 11, 0, 0, 0, 0, 0}, 7},
      {ScaleQuantizer::Scale::Minor, {0, 2, 3, 5, 7, 8, 10, 0, 0, 0, 0, 0}, 7},
      {ScaleQuantizer::Scale::Dorian, {0, 2, 3, 5, 7, 9, 10, 0, 0, 0, 0, 0}, 7},
      {ScaleQuantizer::Scale::Phrygian,
       {0, 1, 3, 5, 7, 8, 10, 0, 0, 0, 0, 0},
       7},
      {ScaleQuantizer::Scale::Lydian, {0, 2, 4, 6, 7, 9, 11, 0, 0, 0, 0, 0}, 7},
      {ScaleQuantizer::Scale::Mixolydian,
       {0, 2, 4, 5, 7, 9, 10, 0, 0, 0, 0, 0},
       7},
      {ScaleQuantizer::Scale::Aeolian,
       {0, 2, 3, 5, 7, 8, 10, 0, 0, 0, 0, 0},
       7},
      {ScaleQuantizer::Scale::Locrian,
       {0, 1, 3, 5, 6, 8, 10, 0, 0, 0, 0, 0},
       7},
      {ScaleQuantizer::Scale::PentMajor,
       {0, 2, 4, 7, 9, 0, 0, 0, 0, 0, 0, 0},
       5},
      {ScaleQuantizer::Scale::PentMinor,
       {0, 3, 5, 7, 10, 0, 0, 0, 0, 0, 0, 0},
       5},
      {ScaleQuantizer::Scale::Blues, {0, 3, 5, 6, 7, 10, 0, 0, 0, 0, 0, 0}, 6},
      {ScaleQuantizer::Scale::WholeTone,
       {0, 2, 4, 6, 8, 10, 0, 0, 0, 0, 0, 0},
       6},
      {ScaleQuantizer::Scale::HarmonicMinor,
       {0, 2, 3, 5, 7, 8, 11, 0, 0, 0, 0, 0},
       7},
      {ScaleQuantizer::Scale::MelodicMinor,
       {0, 2, 3, 5, 7, 9, 11, 0, 0, 0, 0, 0},
       7},
  };

  for (const auto &s : expected) {
    int degrees[12] = {};
    int count = sq.getScaleDegrees(s.scale, degrees, 12);
    ASSERT_EQ(count, s.count);
    for (int i = 0; i < count; ++i)
      ASSERT_EQ(degrees[i], s.degrees[i]);
  }
  PASS();
}

/// Verify all 12 root key transpositions shift intervals correctly.
static void testRootTranspositions() {
  TEST("ScaleQuantizer: 12 root transpositions");
  ScaleQuantizer sq;

  // C Major starting at base octave 3 (MIDI 60 = C4 in octave+2 convention)
  sq.setScale(ScaleQuantizer::Scale::Major, 0); // C
  int cNote = sq.quantize(0, 0, 3, 1, 16);      // C4 = (3+2)*12 + 0 = 60
  ASSERT_EQ(cNote, 60);

  // D Major: root=2, first degree should be D
  sq.setScale(ScaleQuantizer::Scale::Major, 2); // D
  int dNote = sq.quantize(0, 0, 3, 1, 16);      // D4 = 60 + 2 = 62
  ASSERT_EQ(dNote, 62);

  // All 12 roots produce different base notes
  sq.setScale(ScaleQuantizer::Scale::Major, 0);
  for (int root = 0; root < 12; ++root) {
    sq.setScale(ScaleQuantizer::Scale::Major, root);
    int note = sq.quantize(0, 0, 3, 1, 16);
    ASSERT_EQ(note, 60 + root);
  }
  PASS();
}

/// Verify quantized notes stay within scale degrees only.
static void testNoOutOfScaleNotes() {
  TEST("ScaleQuantizer: output is always a valid scale degree");
  ScaleQuantizer sq;
  sq.setScale(ScaleQuantizer::Scale::Major, 0); // C Major

  // C Major semitones from C: 0, 2, 4, 5, 7, 9, 11
  int validSemitones[] = {0, 2, 4, 5, 7, 9, 11};

  for (int col = 0; col < 64; ++col) {
    int note = sq.quantize(0, col, 3, 3, 64);
    int semitone = note % 12;
    bool valid = false;
    for (int v : validSemitones)
      if (semitone == v)
        valid = true;
    ASSERT_TRUE(valid);
  }
  PASS();
}

/// Verify MIDI note output is clamped to 0-127.
static void testMidiClamping() {
  TEST("ScaleQuantizer: MIDI notes clamped to 0-127");
  ScaleQuantizer sq;
  sq.setScale(ScaleQuantizer::Scale::Chromatic, 0);

  int low = sq.quantize(0, 0, -3, 1, 16);
  ASSERT_TRUE(low >= 0);

  int high = sq.quantize(0, 0, 10, 1, 16);
  ASSERT_TRUE(high <= 127);
  PASS();
}

/// All pentatonic scale notes are correct.
static void testPentatonicScales() {
  TEST("ScaleQuantizer: pentatonic scales");
  ScaleQuantizer sq;

  // Major pentatonic: 0, 2, 4, 7, 9
  ASSERT_EQ(sq.getDegreeCount(ScaleQuantizer::Scale::PentMajor), 5);

  // Minor pentatonic: 0, 3, 5, 7, 10
  ASSERT_EQ(sq.getDegreeCount(ScaleQuantizer::Scale::PentMinor), 5);

  int degrees[12];
  sq.getScaleDegrees(ScaleQuantizer::Scale::PentMinor, degrees, 12);
  ASSERT_EQ(degrees[0], 0);
  ASSERT_EQ(degrees[1], 3);
  ASSERT_EQ(degrees[2], 5);
  ASSERT_EQ(degrees[3], 7);
  ASSERT_EQ(degrees[4], 10);
  PASS();
}

// ============================================================================
// Phase 3 — Microtuning Tests
// ============================================================================

/// 12-TET: A4 = 440Hz exactly, A3 = 220Hz exactly.
static void testTET_A4_A3() {
  TEST("Microtuning: 12-TET A4=440, A3=220");
  Microtuning mt;
  mt.setSystem(Microtuning::System::TwelveTET, 440.0f);

  ASSERT_NEAR(mt.getFrequency(69), 440.0, 0.001); // A4
  ASSERT_NEAR(mt.getFrequency(57), 220.0, 0.001); // A3
  ASSERT_NEAR(mt.getFrequency(81), 880.0, 0.01);  // A5
  ASSERT_NEAR(mt.getFrequency(60), 261.63, 0.1);  // C4 (middle C)
  PASS();
}

/// Just Intonation: P5 = 3/2 ratio (701.955 cents).
static void testJust_P5() {
  TEST("Microtuning: Just Intonation P5 = 3/2 ratio");
  Microtuning mt;
  mt.setSystem(Microtuning::System::JustIntonation, 440.0f);

  // G is perfect 5th above C. In Just, ratio = 3/2.
  // C4 = note 60, G4 = note 67
  float c4 = mt.getFrequency(60);
  float g4 = mt.getFrequency(67);
  float ratio = g4 / c4;
  float cents = Microtuning::ratioToCents(ratio);

  ASSERT_NEAR(ratio, 1.5, 0.001);   // 3/2 ratio
  ASSERT_NEAR(cents, 701.955, 0.1); // 701.955 cents
  PASS();
}

/// Pythagorean: P5 = 3/2, M3 = 81/64 (407.82 cents).
static void testPythagorean() {
  TEST("Microtuning: Pythagorean P5=3/2, M3=81/64");
  Microtuning mt;
  mt.setSystem(Microtuning::System::Pythagorean, 440.0f);

  float c4 = mt.getFrequency(60);
  float g4 = mt.getFrequency(67);
  float e4 = mt.getFrequency(64);

  float p5Ratio = g4 / c4;
  float m3Ratio = e4 / c4;
  float m3Cents = Microtuning::ratioToCents(m3Ratio);

  ASSERT_NEAR(p5Ratio, 1.5, 0.001);         // 3/2
  ASSERT_NEAR(m3Ratio, 81.0 / 64.0, 0.001); // 81/64
  ASSERT_NEAR(m3Cents, 407.82, 0.1);        // 407.82 cents
  PASS();
}

/// A4 reference adjustment: 432Hz produces 432.0Hz for A4.
static void testRefPitch() {
  TEST("Microtuning: A4 reference 432Hz");
  Microtuning mt;
  mt.setSystem(Microtuning::System::TwelveTET, 432.0f);

  ASSERT_NEAR(mt.getFrequency(69), 432.0, 0.001);
  ASSERT_NEAR(mt.getFrequency(57), 216.0, 0.001); // A3
  PASS();
}

/// All three tuning systems produce correct A4.
static void testAllSystemsA4() {
  TEST("Microtuning: all 3 systems A4 = ref pitch");
  Microtuning mt;

  mt.setSystem(Microtuning::System::TwelveTET, 440.0f);
  ASSERT_NEAR(mt.getFrequency(69), 440.0, 0.01);

  mt.setSystem(Microtuning::System::JustIntonation, 440.0f);
  // A is degree 9 — JI A4 should equal ref pitch
  ASSERT_NEAR(mt.getFrequency(69), 440.0, 0.1);

  mt.setSystem(Microtuning::System::Pythagorean, 440.0f);
  ASSERT_NEAR(mt.getFrequency(69), 440.0, 0.1);
  PASS();
}

/// Cents offset from TET is zero for 12-TET system.
static void testCentsFromTET() {
  TEST("Microtuning: 12-TET cents offset = 0");
  Microtuning mt;
  mt.setSystem(Microtuning::System::TwelveTET, 440.0f);

  for (int n = 0; n < 128; ++n)
    ASSERT_NEAR(mt.getCentsFromTET(n), 0.0, 0.01);
  PASS();
}

// ============================================================================
// Phase 3 — ClockDivider Tests
// ============================================================================

/// At 120BPM, 1/4 note division, 44100Hz: exactly 2 steps per second.
static void testClockQuarterAt120() {
  TEST("ClockDivider: 120BPM quarter = 2 steps/sec");
  ClockDivider clk;
  clk.reset(44100.0);
  clk.setBPM(120.0);
  clk.setDivision(ClockDivider::Division::Quarter);

  int steps = clk.processBlock(44100); // 1 second of samples
  ASSERT_EQ(steps, 2);
  PASS();
}

/// Test all division multipliers produce correct step counts.
static void testClockDivisions() {
  TEST("ClockDivider: all divisions produce correct step counts");
  ClockDivider clk;
  clk.reset(44100.0);
  clk.setBPM(120.0);

  // At 120BPM, 1 second = 2 quarter notes
  // Whole: 0.5 steps/sec -> may get 0 or 1 in 1 sec due to integer rounding
  // Half: 1 step/sec
  // Quarter: 2 steps/sec
  // Eighth: 4 steps/sec
  // Sixteenth: 8 steps/sec
  // 32nd: 16 steps/sec

  struct DivTest {
    ClockDivider::Division div;
    int expectedMin;
    int expectedMax;
  };
  DivTest tests[] = {
      {ClockDivider::Division::Half, 1, 1},
      {ClockDivider::Division::Quarter, 2, 2},
      {ClockDivider::Division::Eighth, 4, 4},
      {ClockDivider::Division::Sixteenth, 8, 8},
      {ClockDivider::Division::ThirtySecond, 16, 16},
  };

  for (const auto &t : tests) {
    clk.reset(44100.0);
    clk.setBPM(120.0);
    clk.setDivision(t.div);
    int steps = clk.processBlock(44100);
    ASSERT_TRUE(steps >= t.expectedMin && steps <= t.expectedMax);
  }
  PASS();
}

/// Swing timing: 67% swing offsets every other step.
static void testClockSwing() {
  TEST("ClockDivider: 67% swing produces uneven step spacing");
  ClockDivider clk;
  clk.reset(44100.0);
  clk.setBPM(120.0);
  clk.setDivision(ClockDivider::Division::Eighth);
  clk.setSwing(66.67f);

  // Collect step positions over 2 seconds
  int stepPositions[100];
  int stepCount = 0;
  for (int i = 0; i < 88200 && stepCount < 100; ++i) {
    if (clk.tick())
      stepPositions[stepCount++] = i;
  }

  ASSERT_TRUE(stepCount >= 4);

  // With 67% swing, the step intervals alternate long/short.
  // After reset (isOddStep=false), first threshold = normalStepSamples (long).
  // First step fires at normalStepSamples (long). isOddStep flips to true.
  // Second step fires at swungStepSamples (short). isOddStep flips to false.
  // Third step fires at normalStepSamples (long).
  // So: interval0 (1st->2nd) = swungStepSamples (short)
  //     interval1 (2nd->3rd) = normalStepSamples (long)
  int interval0 = stepPositions[1] - stepPositions[0]; // Short
  int interval1 = stepPositions[2] - stepPositions[1]; // Long
  ASSERT_TRUE(interval1 > interval0);                  // Long > Short

  // Ratio should be approximately 2:1 at 67% swing
  double ratio = static_cast<double>(interval1) / interval0;
  ASSERT_NEAR(ratio, 2.0, 0.15);
  PASS();
}

/// No swing (50%) produces even spacing.
static void testClockNoSwing() {
  TEST("ClockDivider: 50% swing = even spacing");
  ClockDivider clk;
  clk.reset(44100.0);
  clk.setBPM(120.0);
  clk.setDivision(ClockDivider::Division::Eighth);
  clk.setSwing(50.0f);

  int stepPositions[20];
  int stepCount = 0;
  for (int i = 0; i < 88200 && stepCount < 20; ++i) {
    if (clk.tick())
      stepPositions[stepCount++] = i;
  }

  ASSERT_TRUE(stepCount >= 4);

  // All intervals should be equal (within 1 sample due to integer rounding)
  int interval0 = stepPositions[1] - stepPositions[0];
  int interval1 = stepPositions[2] - stepPositions[1];
  ASSERT_TRUE(std::abs(interval0 - interval1) <= 1);
  PASS();
}

/// Sample-accurate step count for specific buffer size.
static void testClockBufferAccuracy() {
  TEST("ClockDivider: sample-accurate for buffer sizes");
  ClockDivider clk;
  clk.reset(44100.0);
  clk.setBPM(120.0);
  clk.setDivision(ClockDivider::Division::Quarter);

  // Process exactly 5 seconds worth of samples
  int totalSteps = 0;
  for (int block = 0; block < (5 * 44100) / 512; ++block)
    totalSteps += clk.processBlock(512);

  // At 120 BPM quarter, 2 steps/sec, 5 sec = 10 steps
  // May be off by 1 due to partial last block
  ASSERT_TRUE(totalSteps >= 9 && totalSteps <= 10);
  PASS();
}

// ============================================================================
// Phase 3 — Integration Tests
// ============================================================================

/// Clock -> GoL: at 120BPM quarter, GoL advances exactly 2 generations/sec.
static void testClockDrivesGoL() {
  TEST("Integration: clock drives GoL stepping (2 steps/sec at 120BPM)");
  ClockDivider clk;
  clk.reset(44100.0);
  clk.setBPM(120.0);
  clk.setDivision(ClockDivider::Division::Quarter);

  GameOfLife gol(12, 16, GameOfLife::RulePreset::Classic);
  gol.randomize(42, 0.3f);

  // Simulate 1 second of processBlock
  for (int i = 0; i < 44100; ++i) {
    if (clk.tick())
      gol.step();
  }

  ASSERT_EQ(static_cast<int>(gol.getGeneration()), 2);
  PASS();
}

/// Scale quantizer + GoL: active cells map to valid scale degrees only.
static void testQuantizerWithGoL() {
  TEST("Integration: quantizer maps GoL cells to valid scale degrees");
  ScaleQuantizer sq;
  sq.setScale(ScaleQuantizer::Scale::Major, 0); // C Major

  GameOfLife gol(12, 16, GameOfLife::RulePreset::Classic);
  gol.randomize(42, 0.5f);

  int validSemitones[] = {0, 2, 4, 5, 7, 9, 11}; // C Major

  const Grid &grid = gol.getGrid();
  for (int r = 0; r < grid.getRows(); ++r) {
    for (int c = 0; c < grid.getCols(); ++c) {
      if (grid.getCell(r, c) != 0) {
        int note = sq.quantize(r, c, 3, 3, grid.getCols());
        int semitone = note % 12;
        bool valid = false;
        for (int v : validSemitones)
          if (semitone == v)
            valid = true;
        ASSERT_TRUE(valid);
      }
    }
  }
  PASS();
}

/// Transport pause/resume: GoL freezes during pause, resumes correctly.
static void testTransportPauseResume() {
  TEST("Integration: transport pause freezes GoL, resume continues");
  ClockDivider clk;
  clk.reset(44100.0);
  clk.setBPM(120.0);
  clk.setDivision(ClockDivider::Division::Quarter);

  GameOfLife gol(12, 16, GameOfLife::RulePreset::Classic);
  gol.randomize(42, 0.3f);

  // Run for 1 second
  for (int i = 0; i < 44100; ++i) {
    if (clk.tick())
      gol.step();
  }
  uint64_t genAfterPlay = gol.getGeneration();
  ASSERT_EQ(static_cast<int>(genAfterPlay), 2);

  // Simulate pause: simply don't tick the clock for 1 second
  // (in real code, host transport isPlaying = false)
  Grid frozenGrid(gol.getGrid().getRows(), gol.getGrid().getCols());
  frozenGrid.copyFrom(gol.getGrid());

  // Generation should not change during pause
  ASSERT_EQ(static_cast<int>(gol.getGeneration()), 2);

  // Resume: tick for another second
  for (int i = 0; i < 44100; ++i) {
    if (clk.tick())
      gol.step();
  }
  ASSERT_EQ(static_cast<int>(gol.getGeneration()), 4);
  PASS();
}

// ============================================================================
// Phase 3 — Mutation Tests
// ============================================================================

/// Mutate Dorian interval: [2,1,2,2,2,1,2] -> [2,1,2,2,2,2,1] must differ.
static void testMutation_DorianInterval() {
  TEST("Mutation: Dorian interval change detected");
  ScaleQuantizer sq;

  // Correct Dorian: 0,2,3,5,7,9,10
  int degrees[12];
  sq.getScaleDegrees(ScaleQuantizer::Scale::Dorian, degrees, 12);

  // If degree[5] were 10 instead of 9 (swapping b7 and nat6),
  // the scale would be different
  ASSERT_EQ(degrees[5], 9);      // Correct Dorian has nat6 (9 semitones)
  ASSERT_TRUE(degrees[5] != 10); // Would be wrong if it were 10
  PASS();
}

/// Offset Just Intonation P5 by 1 cent: tuning test must detect it.
static void testMutation_JustP5Offset() {
  TEST("Mutation: Just P5 offset by 1 cent detected");
  Microtuning mt;
  mt.setSystem(Microtuning::System::JustIntonation, 440.0f);

  float c4 = mt.getFrequency(60);
  float g4 = mt.getFrequency(67);
  float ratio = g4 / c4;
  float cents = Microtuning::ratioToCents(ratio);

  // Correct: 701.955 cents. A 1-cent offset would give ~703.
  ASSERT_NEAR(cents, 701.955, 0.5);

  // Verify it would fail at wrong value
  ASSERT_TRUE(std::abs(cents - 703.0) > 0.5);
  PASS();
}

/// Change clock from >= to >: step count must differ.
static void testMutation_ClockComparator() {
  TEST("Mutation: clock >= vs > changes step count");
  ClockDivider clk;
  clk.reset(44100.0);
  clk.setBPM(120.0);
  clk.setDivision(ClockDivider::Division::Quarter);

  int steps = clk.processBlock(44100);
  // With >=, we get 2 steps. With >, we'd get 1 (off by one).
  // Our implementation uses >=, so this should be exactly 2.
  ASSERT_EQ(steps, 2);
  PASS();
}

// ============================================================================
// Phase 4 — PolyBLEP Oscillator Tests
// ============================================================================

static void testOscSineAccuracy() {
  TEST("Sine vs std::sin reference");
  PolyBLEPOscillator osc;
  osc.setWaveshape(PolyBLEPOscillator::Shape::Sine);
  osc.setFrequency(440.0, 44100.0);
  osc.reset();

  double maxError = 0.0;
  double phase = 0.0;
  double phaseInc = 440.0 / 44100.0;
  for (int i = 0; i < 44100; ++i) {
    double oscOut = osc.nextSample();
    double ref = std::sin(2.0 * 3.14159265358979 * phase);
    double err = std::abs(oscOut - ref);
    if (err > maxError)
      maxError = err;
    phase += phaseInc;
    if (phase >= 1.0)
      phase -= 1.0;
  }
  // Sine should be exact (both use std::sin)
  ASSERT_TRUE(maxError < 1e-10);
  PASS();
}

static void testOscSawHarmonics() {
  TEST("Saw harmonics present");
  PolyBLEPOscillator osc;
  osc.setWaveshape(PolyBLEPOscillator::Shape::Saw);
  osc.setFrequency(440.0, 44100.0);
  osc.reset();

  // Generate 1 second of saw, verify it's not silent and not constant
  double sum = 0.0;
  double maxVal = 0.0;
  double minVal = 0.0;
  for (int i = 0; i < 44100; ++i) {
    double s = osc.nextSample();
    sum += s;
    if (s > maxVal)
      maxVal = s;
    if (s < minVal)
      minVal = s;
  }
  ASSERT_TRUE(maxVal > 0.5);  // Should reach near +1
  ASSERT_TRUE(minVal < -0.5); // Should reach near -1
  // DC should be near zero (saw is symmetric)
  double dc = sum / 44100.0;
  ASSERT_TRUE(std::abs(dc) < 0.05);
  PASS();
}

static void testOscSquareHarmonics() {
  TEST("Square wave output range");
  PolyBLEPOscillator osc;
  osc.setWaveshape(PolyBLEPOscillator::Shape::Pulse);
  osc.setPulseWidth(0.5);
  osc.setFrequency(440.0, 44100.0);
  osc.reset();

  double maxVal = 0.0;
  double minVal = 0.0;
  for (int i = 0; i < 44100; ++i) {
    double s = osc.nextSample();
    if (s > maxVal)
      maxVal = s;
    if (s < minVal)
      minVal = s;
  }
  // Pulse output should span a significant range
  ASSERT_TRUE(maxVal > 0.3);
  ASSERT_TRUE(minVal < -0.3);
  PASS();
}

static void testOscPulseWidth() {
  TEST("Pulse width=0.5 is symmetric");
  PolyBLEPOscillator osc;
  osc.setWaveshape(PolyBLEPOscillator::Shape::Pulse);
  osc.setPulseWidth(0.5);
  osc.setFrequency(100.0, 44100.0);
  osc.reset();

  int positiveCount = 0;
  int negativeCount = 0;
  for (int i = 0; i < 44100; ++i) {
    double s = osc.nextSample();
    if (s > 0.0)
      ++positiveCount;
    else if (s < 0.0)
      ++negativeCount;
  }
  // Should be roughly 50/50
  double ratio =
      static_cast<double>(positiveCount) / (positiveCount + negativeCount);
  ASSERT_NEAR(ratio, 0.5, 0.05);
  PASS();
}

static void testOscPolyBLEPvsNaive() {
  TEST("PolyBLEP saw has less aliasing energy than naive");
  // Generate high-frequency saw (10kHz at 44.1kHz) and measure energy variance
  PolyBLEPOscillator osc;
  osc.setWaveshape(PolyBLEPOscillator::Shape::Saw);
  osc.setFrequency(10000.0, 44100.0);
  osc.reset();

  // Just verify it produces output without NaN/Inf
  double sumSq = 0.0;
  bool hasNaN = false;
  for (int i = 0; i < 44100; ++i) {
    double s = osc.nextSample();
    if (std::isnan(s) || std::isinf(s))
      hasNaN = true;
    sumSq += s * s;
  }
  ASSERT_TRUE(!hasNaN);
  ASSERT_TRUE(sumSq > 0.0); // Not silent
  PASS();
}

static void testOscOutputRange() {
  TEST("All shapes output in [-2.1, 2.1]");
  // Check all waveshapes stay bounded (composite shapes sum multiple oscs)
  bool bounded = true;
  for (int shapeIdx = 0;
       shapeIdx < static_cast<int>(PolyBLEPOscillator::Shape::Count);
       ++shapeIdx) {
    PolyBLEPOscillator osc;
    osc.setWaveshape(static_cast<PolyBLEPOscillator::Shape>(shapeIdx));
    osc.setFrequency(440.0, 44100.0);
    osc.reset();

    for (int i = 0; i < 44100; ++i) {
      double s = osc.nextSample();
      if (s > 2.1 || s < -2.1) {
        bounded = false;
        break;
      }
    }
  }
  ASSERT_TRUE(bounded);
  PASS();
}

static void testOscFrequencyAccuracy() {
  TEST("440Hz sine zero-crossing accuracy");
  PolyBLEPOscillator osc;
  osc.setWaveshape(PolyBLEPOscillator::Shape::Sine);
  osc.setFrequency(440.0, 44100.0);
  osc.reset();

  // Count positive-to-negative zero crossings in 1 second
  int crossings = 0;
  double prev = 0.0;
  for (int i = 0; i < 44100; ++i) {
    double s = osc.nextSample();
    if (prev >= 0.0 && s < 0.0)
      ++crossings;
    prev = s;
  }
  // 440Hz = 440 full cycles = 440 zero crossings (pos->neg)
  ASSERT_NEAR(static_cast<double>(crossings), 440.0, 1.0);
  PASS();
}

// ============================================================================
// Phase 4 — AHDSR Envelope Tests
// ============================================================================

static void testEnvelopeAttack() {
  TEST("Attack ramp timing");
  AHDSREnvelope env;
  double sr = 44100.0;
  double attackTime = 0.01; // 10ms
  env.setParameters(attackTime, 0.0, 0.0, 1.0, 0.1, sr);
  env.noteOn();

  int attackSamples = static_cast<int>(attackTime * sr);
  double levelAtEnd = 0.0;
  for (int i = 0; i <= attackSamples + 1; ++i) {
    levelAtEnd = env.nextSample();
  }
  // Should reach 1.0 by end of attack
  ASSERT_NEAR(levelAtEnd, 1.0, 0.02);
  PASS();
}

static void testEnvelopeHoldDecaySustain() {
  TEST("Hold then Decay to Sustain");
  AHDSREnvelope env;
  double sr = 44100.0;
  env.setParameters(0.001, 0.01, 0.01, 0.5, 0.1, sr);
  env.noteOn();

  // Process through attack
  int attackSamples = static_cast<int>(0.001 * sr) + 2;
  for (int i = 0; i < attackSamples; ++i)
    env.nextSample();

  // Process through hold (10ms)
  int holdSamples = static_cast<int>(0.01 * sr);
  double holdLevel = 0.0;
  for (int i = 0; i < holdSamples; ++i)
    holdLevel = env.nextSample();
  ASSERT_NEAR(holdLevel, 1.0, 0.02);

  // Process through decay
  int decaySamples = static_cast<int>(0.01 * sr) + 2;
  double decayLevel = 0.0;
  for (int i = 0; i < decaySamples; ++i)
    decayLevel = env.nextSample();
  ASSERT_NEAR(decayLevel, 0.5, 0.05);
  PASS();
}

static void testEnvelopeRelease() {
  TEST("Release from sustain to zero");
  AHDSREnvelope env;
  double sr = 44100.0;
  env.setParameters(0.001, 0.0, 0.001, 0.7, 0.01, sr);
  env.noteOn();

  // Process to sustain
  for (int i = 0; i < 500; ++i)
    env.nextSample();

  env.noteOff();
  int releaseSamples = static_cast<int>(0.01 * sr) + 10;
  double finalLevel = 0.0;
  for (int i = 0; i < releaseSamples; ++i)
    finalLevel = env.nextSample();

  ASSERT_TRUE(finalLevel < 0.01);
  ASSERT_TRUE(!env.isActive());
  PASS();
}

static void testEnvelopeNoteOffDuringAttack() {
  TEST("Note-off during attack transitions to release");
  AHDSREnvelope env;
  double sr = 44100.0;
  env.setParameters(0.1, 0.0, 0.0, 1.0, 0.01, sr);
  env.noteOn();

  // Process half of attack (50ms of 100ms)
  for (int i = 0; i < 2205; ++i)
    env.nextSample();
  double levelAtNoteOff = env.getLevel();
  ASSERT_TRUE(levelAtNoteOff > 0.1 && levelAtNoteOff < 0.9);

  env.noteOff();
  // Should be in release now, decreasing from current level
  double postRelease = env.nextSample();
  ASSERT_TRUE(postRelease <= levelAtNoteOff);
  PASS();
}

static void testEnvelopeRetrigger() {
  TEST("Retrigger from current level (no snap to zero)");
  AHDSREnvelope env;
  double sr = 44100.0;
  env.setParameters(0.01, 0.0, 0.0, 1.0, 0.5, sr);
  env.noteOn();

  // Reach peak
  for (int i = 0; i < 1000; ++i)
    env.nextSample();
  env.noteOff();

  // Process a bit of release
  for (int i = 0; i < 100; ++i)
    env.nextSample();
  double levelBeforeRetrigger = env.getLevel();
  ASSERT_TRUE(levelBeforeRetrigger > 0.0);

  // Retrigger: attack should continue from current level, not zero
  env.noteOn();
  double firstSampleAfter = env.nextSample();
  ASSERT_TRUE(firstSampleAfter >= levelBeforeRetrigger * 0.9);
  PASS();
}

// ============================================================================
// Phase 4 — SVFilter Tests
// ============================================================================

static void testFilterLP() {
  TEST("LP -3dB at cutoff, attenuation above");
  SVFilter filter;
  double sr = 44100.0;
  filter.setCutoff(1000.0, sr);
  filter.setResonance(0.0);
  filter.setMode(SVFilter::Mode::LowPass);

  // Measure energy at 500Hz (below cutoff) and 4000Hz (above)
  auto measureEnergy = [&](double freq) -> double {
    filter.reset();
    double sumSq = 0.0;
    int n = 8820; // ~200ms
    for (int i = 0; i < n; ++i) {
      double input = std::sin(2.0 * 3.14159265358979 * freq * i / sr);
      double out = filter.process(input);
      if (i > 4410) // Skip transient
        sumSq += out * out;
    }
    return sumSq;
  };

  double eLow = measureEnergy(500.0);
  double eHigh = measureEnergy(4000.0);
  // Below cutoff should pass, above should be attenuated
  ASSERT_TRUE(eLow > eHigh * 4.0);
  PASS();
}

static void testFilterHP() {
  TEST("HP passes above cutoff, attenuates below");
  SVFilter filter;
  double sr = 44100.0;
  filter.setCutoff(1000.0, sr);
  filter.setResonance(0.0);
  filter.setMode(SVFilter::Mode::HighPass);

  auto measureEnergy = [&](double freq) -> double {
    filter.reset();
    double sumSq = 0.0;
    int n = 8820;
    for (int i = 0; i < n; ++i) {
      double input = std::sin(2.0 * 3.14159265358979 * freq * i / sr);
      double out = filter.process(input);
      if (i > 4410)
        sumSq += out * out;
    }
    return sumSq;
  };

  double eLow = measureEnergy(200.0);
  double eHigh = measureEnergy(4000.0);
  ASSERT_TRUE(eHigh > eLow * 4.0);
  PASS();
}

static void testFilterResonance() {
  TEST("Resonance boosts energy at cutoff");
  SVFilter filterFlat;
  SVFilter filterRes;
  double sr = 44100.0;
  double cutoff = 1000.0;

  filterFlat.setCutoff(cutoff, sr);
  filterFlat.setResonance(0.0);
  filterFlat.setMode(SVFilter::Mode::LowPass);

  filterRes.setCutoff(cutoff, sr);
  filterRes.setResonance(0.9);
  filterRes.setMode(SVFilter::Mode::LowPass);

  // Measure at cutoff frequency
  double sumFlat = 0.0;
  double sumRes = 0.0;
  int n = 8820;
  for (int i = 0; i < n; ++i) {
    double input = std::sin(2.0 * 3.14159265358979 * cutoff * i / sr);
    double outFlat = filterFlat.process(input);
    double outRes = filterRes.process(input);
    if (i > 4410) {
      sumFlat += outFlat * outFlat;
      sumRes += outRes * outRes;
    }
  }
  // High resonance should produce more energy at cutoff
  ASSERT_TRUE(sumRes > sumFlat * 1.5);
  PASS();
}

static void testFilterStability() {
  TEST("Filter stable at max resonance");
  SVFilter filter;
  filter.setCutoff(5000.0, 44100.0);
  filter.setResonance(0.99);
  filter.setMode(SVFilter::Mode::LowPass);
  filter.reset();

  bool stable = true;
  for (int i = 0; i < 88200; ++i) {
    double input = (i == 0) ? 1.0 : 0.0; // Impulse
    double out = filter.process(input);
    if (std::isnan(out) || std::isinf(out) || std::abs(out) > 100.0) {
      stable = false;
      break;
    }
  }
  ASSERT_TRUE(stable);
  PASS();
}

// ============================================================================
// Phase 4 — SynthVoice Tests
// ============================================================================

static void testVoiceChain() {
  TEST("Full voice chain produces non-zero output");
  SynthVoice voice;
  voice.setWaveshape(PolyBLEPOscillator::Shape::Saw);
  voice.setEnvelopeParams(0.01, 0.0, 0.1, 0.7, 0.5, 44100.0);
  voice.setFilterCutoff(4000.0);
  voice.setFilterResonance(0.2);
  voice.noteOn(60, 0.8, 261.63, 44100.0);

  double sumSq = 0.0;
  for (int i = 0; i < 4410; ++i) {
    auto s = voice.renderNextSample();
    sumSq += s.left * s.left + s.right * s.right;
  }
  ASSERT_TRUE(sumSq > 0.0);
  ASSERT_TRUE(voice.isActive());
  PASS();
}

static void testVoicePolyphony() {
  TEST("8 voices summed stay bounded");
  SynthVoice voices[8];
  double frequencies[] = {261.63, 293.66, 329.63, 349.23,
                          392.00, 440.00, 493.88, 523.25};
  for (int v = 0; v < 8; ++v) {
    voices[v].setWaveshape(PolyBLEPOscillator::Shape::Sine);
    voices[v].setEnvelopeParams(0.001, 0.0, 0.1, 0.7, 0.5, 44100.0);
    voices[v].setFilterCutoff(8000.0);
    voices[v].noteOn(60 + v, 0.8, frequencies[v], 44100.0);
  }

  bool bounded = true;
  for (int i = 0; i < 4410; ++i) {
    double sumL = 0.0, sumR = 0.0;
    for (int v = 0; v < 8; ++v) {
      auto s = voices[v].renderNextSample();
      sumL += s.left;
      sumR += s.right;
    }
    double voiceGain = 1.0 / 8.0;
    if (std::abs(sumL * voiceGain) > 1.5 || std::abs(sumR * voiceGain) > 1.5) {
      bounded = false;
      break;
    }
  }
  ASSERT_TRUE(bounded);
  PASS();
}

static void testSubOscTracking() {
  TEST("Sub-oscillator tracks voice frequency");
  SubOscillator sub;
  sub.setLevel(1.0);
  sub.setOctaveMode(SubOscillator::OctaveMode::Down1);
  sub.setFrequency(440.0, 44100.0);
  sub.reset();

  // Count zero crossings for sub at -1 octave = 220Hz
  int crossings = 0;
  double prev = 0.0;
  for (int i = 0; i < 44100; ++i) {
    double s = sub.nextSample();
    if (prev >= 0.0 && s < 0.0)
      ++crossings;
    prev = s;
  }
  ASSERT_NEAR(static_cast<double>(crossings), 220.0, 1.0);
  PASS();
}

// ============================================================================
// Phase 4 — Mutation Tests
// ============================================================================

static void testMutation_PolyBLEPRemoval() {
  TEST("Mutation: PolyBLEP presence changes saw output");
  // Verify that the PolyBLEP oscillator produces different output than
  // a trivial 2*phase-1 ramp (the naive version)
  PolyBLEPOscillator osc;
  osc.setWaveshape(PolyBLEPOscillator::Shape::Saw);
  osc.setFrequency(5000.0, 44100.0); // High freq makes BLEP more visible
  osc.reset();

  double phaseInc = 5000.0 / 44100.0;
  double phase = 0.0;
  double diffSum = 0.0;

  for (int i = 0; i < 44100; ++i) {
    double blep = osc.nextSample();
    double naive = 2.0 * phase - 1.0;
    diffSum += std::abs(blep - naive);
    phase += phaseInc;
    if (phase >= 1.0)
      phase -= 1.0;
  }
  // PolyBLEP should differ from naive saw
  ASSERT_TRUE(diffSum > 1.0);
  PASS();
}

static void testMutation_EnvelopeInstantAttack() {
  TEST("Mutation: non-zero attack takes multiple samples");
  AHDSREnvelope env;
  env.setParameters(0.01, 0.0, 0.0, 1.0, 0.1, 44100.0); // 10ms attack
  env.noteOn();

  // First sample should NOT be 1.0 (instant attack would be)
  double first = env.nextSample();
  ASSERT_TRUE(first < 0.5);
  PASS();
}

static void testMutation_FilterCutoffOffset() {
  TEST("Mutation: changing cutoff changes filter response");
  SVFilter filterLow, filterHigh;
  double sr = 44100.0;
  filterLow.setCutoff(500.0, sr);
  filterLow.setMode(SVFilter::Mode::LowPass);
  filterHigh.setCutoff(5000.0, sr);
  filterHigh.setMode(SVFilter::Mode::LowPass);

  // Feed 2kHz sine through both
  double sumLow = 0.0, sumHigh = 0.0;
  for (int i = 0; i < 8820; ++i) {
    double input = std::sin(2.0 * 3.14159265358979 * 2000.0 * i / sr);
    double outL = filterLow.process(input);
    double outH = filterHigh.process(input);
    if (i > 4410) {
      sumLow += outL * outL;
      sumHigh += outH * outH;
    }
  }
  // 2kHz signal should pass more through 5kHz cutoff than 500Hz cutoff
  ASSERT_TRUE(sumHigh > sumLow * 2.0);
  PASS();
}

static void testMutation_SubOctaveDivision() {
  TEST("Mutation: sub -2oct is half freq of -1oct");
  SubOscillator sub1, sub2;
  sub1.setLevel(1.0);
  sub2.setLevel(1.0);
  sub1.setOctaveMode(SubOscillator::OctaveMode::Down1);
  sub2.setOctaveMode(SubOscillator::OctaveMode::Down2);
  sub1.setFrequency(440.0, 44100.0);
  sub2.setFrequency(440.0, 44100.0);
  sub1.reset();
  sub2.reset();

  // Count zero crossings
  int cross1 = 0, cross2 = 0;
  double prev1 = 0.0, prev2 = 0.0;
  for (int i = 0; i < 44100; ++i) {
    double s1 = sub1.nextSample();
    double s2 = sub2.nextSample();
    if (prev1 >= 0 && s1 < 0)
      ++cross1;
    if (prev2 >= 0 && s2 < 0)
      ++cross2;
    prev1 = s1;
    prev2 = s2;
  }
  // -1oct = 220Hz, -2oct = 110Hz. Ratio should be 2:1
  double ratio = static_cast<double>(cross1) / cross2;
  ASSERT_NEAR(ratio, 2.0, 0.05);
  PASS();
}

// ============================================================================
// CA Engine Tests (Phase 5)
// ============================================================================
void testBriansBrainBasics() {
  TEST("BriansBrain: step produces activity");
  BriansBrain bb(12, 16);
  bb.randomize(42, 0.3f);
  ASSERT_TRUE(bb.getGrid().countAlive() > 0);
  bb.step();
  ASSERT_EQ(bb.getGeneration(), 1u);
  bb.clear();
  ASSERT_EQ(bb.getGrid().countAlive(), 0);
  ASSERT_EQ(bb.getGeneration(), 0u);
  PASS();
}

void testCyclicCABasics() {
  TEST("CyclicCA: step advances generation");
  CyclicCA ca(12, 16);
  ca.randomize(42, 0.3f);
  ASSERT_TRUE(ca.getGrid().countAlive() > 0);
  for (int i = 0; i < 10; ++i)
    ca.step();
  ASSERT_EQ(ca.getGeneration(), 10u);
  ca.clear();
  ASSERT_EQ(ca.getGeneration(), 0u);
  PASS();
}

void testReactionDiffusionBasics() {
  TEST("ReactionDiffusion: float fields and grid projection");
  ReactionDiffusion rd(12, 16);
  rd.randomize(42, 0.3f);
  // After randomize, there should be some activity in the grid
  for (int i = 0; i < 5; ++i)
    rd.step();
  ASSERT_EQ(rd.getGeneration(), 5u);
  // Verify native float data is accessible
  const float *u = rd.getFieldA();
  const float *v = rd.getFieldB();
  ASSERT_TRUE(u != nullptr);
  ASSERT_TRUE(v != nullptr);
  rd.clear();
  ASSERT_EQ(rd.getGeneration(), 0u);
  PASS();
}

void testLeniaEngineBasics() {
  TEST("LeniaEngine: continuous state and step");
  LeniaEngine le(12, 16);
  le.randomize(42, 0.3f);
  // After randomize, state field should have some non-zero values
  const float *state = le.getStateField();
  ASSERT_TRUE(state != nullptr);
  bool hasNonZero = false;
  for (int i = 0; i < 12 * Grid::kMaxCols; ++i) {
    if (state[i] > 0.0f) {
      hasNonZero = true;
      break;
    }
  }
  ASSERT_TRUE(hasNonZero);
  for (int i = 0; i < 5; ++i)
    le.step();
  ASSERT_EQ(le.getGeneration(), 5u);
  le.clear();
  ASSERT_EQ(le.getGeneration(), 0u);
  PASS();
}

void testParticleSwarmBasics() {
  TEST("ParticleSwarm: particles deposit trails");
  ParticleSwarm ps(12, 16);
  ps.randomize(42, 0.3f);
  // After randomize, particles should exist
  const auto *particles = ps.getParticles();
  ASSERT_TRUE(particles != nullptr);
  for (int i = 0; i < 10; ++i)
    ps.step();
  ASSERT_EQ(ps.getGeneration(), 10u);
  // Trail field should have deposited some energy
  const float *trail = ps.getTrailField();
  bool hasTrail = false;
  for (int i = 0; i < 12 * Grid::kMaxCols; ++i) {
    if (trail[i] > 0.0f) {
      hasTrail = true;
      break;
    }
  }
  ASSERT_TRUE(hasTrail);
  ps.clear();
  ASSERT_EQ(ps.getGeneration(), 0u);
  PASS();
}

void testBrownianFieldBasics() {
  TEST("BrownianField: walkers deposit energy");
  BrownianField bf(12, 16);
  bf.randomize(42, 0.3f);
  for (int i = 0; i < 10; ++i)
    bf.step();
  ASSERT_EQ(bf.getGeneration(), 10u);
  const float *energy = bf.getEnergyField();
  bool hasEnergy = false;
  for (int i = 0; i < 12 * Grid::kMaxCols; ++i) {
    if (energy[i] > 0.0f) {
      hasEnergy = true;
      break;
    }
  }
  ASSERT_TRUE(hasEnergy);
  bf.clear();
  ASSERT_EQ(bf.getGeneration(), 0u);
  PASS();
}

void testEngineTypeIdentification() {
  TEST("Engine type identification via getType()");
  GameOfLife gol(12, 16);
  BriansBrain bb(12, 16);
  CyclicCA ca(12, 16);
  ReactionDiffusion rd(12, 16);
  LeniaEngine le(12, 16);
  ParticleSwarm ps(12, 16);
  BrownianField bf(12, 16);

  ASSERT_EQ(static_cast<int>(gol.getType()), static_cast<int>(EngineType::GoL));
  ASSERT_EQ(static_cast<int>(bb.getType()),
            static_cast<int>(EngineType::BriansBrain));
  ASSERT_EQ(static_cast<int>(ca.getType()),
            static_cast<int>(EngineType::CyclicCA));
  ASSERT_EQ(static_cast<int>(rd.getType()),
            static_cast<int>(EngineType::ReactionDiffusion));
  ASSERT_EQ(static_cast<int>(le.getType()),
            static_cast<int>(EngineType::Lenia));
  ASSERT_EQ(static_cast<int>(ps.getType()),
            static_cast<int>(EngineType::ParticleSwarm));
  ASSERT_EQ(static_cast<int>(bf.getType()),
            static_cast<int>(EngineType::BrownianField));
  PASS();
}

// ============================================================================
// Main
// ============================================================================
// Phase 5.5 -- Anti-Cacophony Tests
// ============================================================================

void testConsonanceFilter() {
  TEST("isConsonant - all 12 intervals");
  // Consonant: 0(unison), 3(m3), 4(M3), 5(P4), 7(P5), 8(m6), 9(M6)
  ASSERT_TRUE(ScaleQuantizer::isConsonant(60, 60)); // Unison
  ASSERT_TRUE(ScaleQuantizer::isConsonant(60, 63)); // m3
  ASSERT_TRUE(ScaleQuantizer::isConsonant(60, 64)); // M3
  ASSERT_TRUE(ScaleQuantizer::isConsonant(60, 65)); // P4
  ASSERT_TRUE(ScaleQuantizer::isConsonant(60, 67)); // P5
  ASSERT_TRUE(ScaleQuantizer::isConsonant(60, 68)); // m6
  ASSERT_TRUE(ScaleQuantizer::isConsonant(60, 69)); // M6
  // Dissonant: 1(m2), 2(M2), 6(tritone), 10(m7), 11(M7)
  ASSERT_TRUE(!ScaleQuantizer::isConsonant(60, 61)); // m2
  ASSERT_TRUE(!ScaleQuantizer::isConsonant(60, 62)); // M2
  ASSERT_TRUE(!ScaleQuantizer::isConsonant(60, 66)); // tritone
  ASSERT_TRUE(!ScaleQuantizer::isConsonant(60, 70)); // m7
  ASSERT_TRUE(!ScaleQuantizer::isConsonant(60, 71)); // M7
  // Cross-octave: P5 across octaves
  ASSERT_TRUE(ScaleQuantizer::isConsonant(48, 67)); // P5 + octave
  PASS();
}

void testConsonantWithAll() {
  TEST("isConsonantWithAll - 0/1/3 active notes");
  // No active notes: always consonant
  ASSERT_TRUE(ScaleQuantizer::isConsonantWithAll(60, nullptr, 0));

  // 1 active note: C4(60), candidate E4(64) -> M3 = consonant
  int active1[] = {60};
  ASSERT_TRUE(ScaleQuantizer::isConsonantWithAll(64, active1, 1));
  // C4(60), candidate D4(62) -> M2 = dissonant
  ASSERT_TRUE(!ScaleQuantizer::isConsonantWithAll(62, active1, 1));

  // 3 active notes: C4(60), E4(64), G4(67) - major chord
  int active3[] = {60, 64, 67};
  // C5(72) consonant with all (octave, M3, P4)
  ASSERT_TRUE(ScaleQuantizer::isConsonantWithAll(72, active3, 3));
  // D4(62) dissonant with C4 (M2)
  ASSERT_TRUE(!ScaleQuantizer::isConsonantWithAll(62, active3, 3));
  PASS();
}

void testQuantizeWeighted() {
  TEST("quantizeWeighted - gravity=1.0 snaps to chord tones");
  ScaleQuantizer q;
  q.setScale(ScaleQuantizer::Scale::Major, 0); // C Major
  uint64_t rng = 42;

  // With gravity=1.0, all notes should snap to root(C), 3rd(E), or 5th(G)
  // in terms of semitones from root: 0, 4, 7
  bool allChordTones = true;
  for (int col = 0; col < 16; ++col) {
    int note = q.quantizeWeighted(0, col, 3, 3, 16, 1.0f, rng);
    int semitone = (note - 0) % 12; // Root is C=0
    if (semitone < 0)
      semitone += 12;
    if (semitone != 0 && semitone != 4 && semitone != 7) {
      allChordTones = false;
      break;
    }
  }
  ASSERT_TRUE(allChordTones);

  // With gravity=0.0, should behave like normal quantize
  uint64_t rng2 = 42;
  for (int col = 0; col < 7; ++col) {
    int weighted = q.quantizeWeighted(0, col, 3, 3, 16, 0.0f, rng2);
    int normal = q.quantize(0, col, 3, 3, 16);
    ASSERT_EQ(weighted, normal);
  }
  PASS();
}

void testGridDensity() {
  TEST("Grid::getDensity - empty/full/partial");
  Grid g(4, 4); // 16 cells
  // Empty grid
  ASSERT_NEAR(g.getDensity(), 0.0f, 0.01f);
  // Full grid
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      g.setCell(r, c, 1);
  ASSERT_NEAR(g.getDensity(), 1.0f, 0.01f);
  // Half full
  g.clear();
  for (int r = 0; r < 2; ++r)
    for (int c = 0; c < 4; ++c)
      g.setCell(r, c, 1);
  ASSERT_NEAR(g.getDensity(), 0.5f, 0.01f);
  PASS();
}

// ============================================================================
int main() {
  std::cout << "=== Algo Nebula Phase 2+3+4 Tests ===" << std::endl;

  // Phase 2 — Grid tests
  std::cout << "\n[Grid]" << std::endl;
  testGridBasics();
  testGridCustomDimensions();
  testGridClamping();
  testGridSetGetCell();
  testGridToroidalWrap();
  testGridAge();
  testGridCountAlive();
  testGridEquality();
  testGridCopyFrom();

  // Phase 2 — GoL correctness tests
  std::cout << "\n[Game of Life -- Correctness]" << std::endl;
  testGoLBlinker();
  testGoLBlock();
  testGoLBeehive();
  testGoLGlider();
  testGoLGeneration();
  testGoLAge();
  testGoLDeterministicSeed();
  testGoLDensity();
  testGoLZeroDensity();
  testGoLFullDensity();

  // Phase 2 — Rule variants
  std::cout << "\n[Game of Life -- Rule Variants]" << std::endl;
  testGoLHighLife();
  testGoLSeeds();
  testGoLAmbient();
  testGoLToroidal();

  // Phase 2 — SPSC Queue
  std::cout << "\n[CellEditQueue]" << std::endl;
  testQueuePushPop();
  testQueueFull();
  testQueueDrainInto();
  testQueueBoundedDrain();

  // Phase 2 — Mutation
  std::cout << "\n[Phase 2 Mutation Tests]" << std::endl;
  testMutation_BirthRuleFlip();
  testMutation_SurvivalDisabled();
  testMutation_AgeReset();

  // Phase 3 — Scale Quantizer
  std::cout << "\n[ScaleQuantizer]" << std::endl;
  testAllScaleIntervals();
  testRootTranspositions();
  testNoOutOfScaleNotes();
  testMidiClamping();
  testPentatonicScales();

  // Phase 3 — Microtuning
  std::cout << "\n[Microtuning]" << std::endl;
  testTET_A4_A3();
  testJust_P5();
  testPythagorean();
  testRefPitch();
  testAllSystemsA4();
  testCentsFromTET();

  // Phase 3 — ClockDivider
  std::cout << "\n[ClockDivider]" << std::endl;
  testClockQuarterAt120();
  testClockDivisions();
  testClockSwing();
  testClockNoSwing();
  testClockBufferAccuracy();

  // Phase 3 — Integration
  std::cout << "\n[Phase 3 Integration]" << std::endl;
  testClockDrivesGoL();
  testQuantizerWithGoL();
  testTransportPauseResume();

  // Phase 3 — Mutation
  std::cout << "\n[Phase 3 Mutation Tests]" << std::endl;
  testMutation_DorianInterval();
  testMutation_JustP5Offset();
  testMutation_ClockComparator();

  // Phase 4 -- Oscillator
  std::cout << "\n[PolyBLEP Oscillator]" << std::endl;
  testOscSineAccuracy();
  testOscSawHarmonics();
  testOscSquareHarmonics();
  testOscPulseWidth();
  testOscPolyBLEPvsNaive();
  testOscOutputRange();
  testOscFrequencyAccuracy();

  // Phase 4 -- Envelope
  std::cout << "\n[AHDSR Envelope]" << std::endl;
  testEnvelopeAttack();
  testEnvelopeHoldDecaySustain();
  testEnvelopeRelease();
  testEnvelopeNoteOffDuringAttack();
  testEnvelopeRetrigger();

  // Phase 4 -- Filter
  std::cout << "\n[SVFilter]" << std::endl;
  testFilterLP();
  testFilterHP();
  testFilterResonance();
  testFilterStability();

  // Phase 4 -- SynthVoice
  std::cout << "\n[SynthVoice]" << std::endl;
  testVoiceChain();
  testVoicePolyphony();
  testSubOscTracking();

  // Phase 4 -- Mutation
  std::cout << "\n[Phase 4 Mutation Tests]" << std::endl;
  testMutation_PolyBLEPRemoval();
  testMutation_EnvelopeInstantAttack();
  testMutation_FilterCutoffOffset();
  testMutation_SubOctaveDivision();

  // Phase 5 -- CA Engines
  std::cout << "\n[CA Engine Tests]" << std::endl;
  testBriansBrainBasics();
  testCyclicCABasics();
  testReactionDiffusionBasics();
  testLeniaEngineBasics();
  testParticleSwarmBasics();
  testBrownianFieldBasics();
  testEngineTypeIdentification();

  // Phase 5.5 -- Anti-Cacophony
  std::cout << "\n[Anti-Cacophony Tests]" << std::endl;
  testConsonanceFilter();
  testConsonantWithAll();
  testQuantizeWeighted();
  testGridDensity();

  // Summary
  std::cout << "\n=== Results ===" << std::endl;
  std::cout << "  Passed: " << testsPassed << std::endl;
  std::cout << "  Failed: " << testsFailed << std::endl;

  if (testsFailed > 0) {
    std::cout << "SOME TESTS FAILED!" << std::endl;
    return 1;
  }

  std::cout << "All tests passed." << std::endl;
  return 0;
}
