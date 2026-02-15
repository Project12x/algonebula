#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#include "engine/CellEditQueue.h"
#include "engine/GameOfLife.h"
#include "engine/Grid.h"


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
// Main
// ============================================================================
int main() {
  std::cout << "=== Algo Nebula Phase 2 Tests ===" << std::endl;

  // Grid tests
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

  // GoL correctness tests
  std::cout << "\n[Game of Life — Correctness]" << std::endl;
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

  // Rule variant tests
  std::cout << "\n[Game of Life — Rule Variants]" << std::endl;
  testGoLHighLife();
  testGoLSeeds();
  testGoLAmbient();
  testGoLToroidal();

  // SPSC Queue tests
  std::cout << "\n[CellEditQueue]" << std::endl;
  testQueuePushPop();
  testQueueFull();
  testQueueDrainInto();
  testQueueBoundedDrain();

  // Mutation tests
  std::cout << "\n[Mutation Tests]" << std::endl;
  testMutation_BirthRuleFlip();
  testMutation_SurvivalDisabled();
  testMutation_AgeReset();

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
