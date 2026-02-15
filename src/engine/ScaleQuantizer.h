#pragma once

#include <array>
#include <cstdint>

/// Scale quantizer: maps grid cell positions to valid scale degrees.
/// 15 scales x 12 root keys, pre-computed pitch tables.
/// All lookups are O(1) array indexing — RT-safe.
class ScaleQuantizer {
public:
  /// Scale definitions using semitone intervals from root.
  enum class Scale : int {
    Chromatic = 0,
    Major,
    Minor,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Aeolian,
    Locrian,
    PentMajor,
    PentMinor,
    Blues,
    WholeTone,
    HarmonicMinor,
    MelodicMinor,
    Count
  };

  static constexpr int kScaleCount = static_cast<int>(Scale::Count);
  static constexpr int kMaxDegreesPerOctave = 12;

  ScaleQuantizer() { buildAllTables(); }

  /// Set current scale and root key (0=C, 1=C#, ..., 11=B).
  void setScale(Scale scale, int rootKey) {
    currentScale = scale;
    currentRoot = rootKey % 12;
    if (currentRoot < 0)
      currentRoot += 12;
  }

  /// Quantize a raw grid position to a MIDI note number.
  /// @param cellRow  Row position in grid (used with octave range).
  /// @param cellCol  Column position in grid (used as pitch offset).
  /// @param baseOctave  Base MIDI octave (e.g., 3 = C3 = MIDI 48).
  /// @param octaveSpan  How many octaves the grid spans.
  /// @param gridCols  Total columns in grid.
  /// @return MIDI note number (0-127), clamped.
  int quantize(int cellRow, int cellCol, int baseOctave, int octaveSpan,
               int gridCols) const {
    if (gridCols <= 0)
      return 60; // C4 fallback

    const auto &degrees = scaleDegrees[static_cast<int>(currentScale)];
    int degreeCount = scaleDegreeCounts[static_cast<int>(currentScale)];

    if (degreeCount <= 0)
      degreeCount = 12; // Chromatic fallback

    // Map column to scale degree index
    int degreeIndex = cellCol % degreeCount;

    // Map row to octave offset
    int octaveOffset = 0;
    if (octaveSpan > 0)
      octaveOffset = (cellCol / degreeCount) % octaveSpan;

    int semitone = degrees[degreeIndex] + currentRoot;
    int midiNote = (baseOctave + 2) * 12 + semitone + octaveOffset * 12;

    // Clamp to valid MIDI range
    if (midiNote < 0)
      midiNote = 0;
    if (midiNote > 127)
      midiNote = 127;

    return midiNote;
  }

  /// Get the interval pattern for a scale (for testing).
  /// Returns number of degrees filled in outDegrees.
  int getScaleDegrees(Scale scale, int *outDegrees, int maxOut) const {
    int idx = static_cast<int>(scale);
    int count = scaleDegreeCounts[idx];
    int n = count < maxOut ? count : maxOut;
    for (int i = 0; i < n; ++i)
      outDegrees[i] = scaleDegrees[idx][i];
    return n;
  }

  /// Get the number of degrees in a scale.
  int getDegreeCount(Scale scale) const {
    return scaleDegreeCounts[static_cast<int>(scale)];
  }

  /// Get current root key.
  int getCurrentRoot() const { return currentRoot; }

  /// Get current scale.
  Scale getCurrentScale() const { return currentScale; }

private:
  void buildAllTables() {
    // Chromatic: all 12 semitones
    setDegrees(Scale::Chromatic, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});

    // Major modes (diatonic)
    setDegrees(Scale::Major, {0, 2, 4, 5, 7, 9, 11}); // W-W-H-W-W-W-H
    setDegrees(Scale::Minor,
               {0, 2, 3, 5, 7, 8, 10}); // W-H-W-W-H-W-W (natural minor)
    setDegrees(Scale::Dorian, {0, 2, 3, 5, 7, 9, 10});     // W-H-W-W-W-H-W
    setDegrees(Scale::Phrygian, {0, 1, 3, 5, 7, 8, 10});   // H-W-W-W-H-W-W
    setDegrees(Scale::Lydian, {0, 2, 4, 6, 7, 9, 11});     // W-W-W-H-W-W-H
    setDegrees(Scale::Mixolydian, {0, 2, 4, 5, 7, 9, 10}); // W-W-H-W-W-H-W
    setDegrees(Scale::Aeolian, {0, 2, 3, 5, 7, 8, 10});    // = Natural Minor
    setDegrees(Scale::Locrian, {0, 1, 3, 5, 6, 8, 10});    // H-W-W-H-W-W-W

    // Pentatonic
    setDegrees(Scale::PentMajor, {0, 2, 4, 7, 9});  // 1-2-3-5-6
    setDegrees(Scale::PentMinor, {0, 3, 5, 7, 10}); // 1-b3-4-5-b7

    // Other
    setDegrees(Scale::Blues, {0, 3, 5, 6, 7, 10});     // 1-b3-4-#4-5-b7
    setDegrees(Scale::WholeTone, {0, 2, 4, 6, 8, 10}); // W-W-W-W-W-W

    // Extended minor
    setDegrees(Scale::HarmonicMinor,
               {0, 2, 3, 5, 7, 8, 11}); // W-H-W-W-H-Aug2-H
    setDegrees(Scale::MelodicMinor,
               {0, 2, 3, 5, 7, 9, 11}); // W-H-W-W-W-W-H (ascending)
  }

  void setDegrees(Scale scale, std::initializer_list<int> degrees) {
    int idx = static_cast<int>(scale);
    int i = 0;
    for (int d : degrees) {
      if (i >= kMaxDegreesPerOctave)
        break;
      scaleDegrees[idx][i++] = d;
    }
    scaleDegreeCounts[idx] = i;
  }

  // Pre-computed scale degree tables
  int scaleDegrees[kScaleCount][kMaxDegreesPerOctave] = {};
  int scaleDegreeCounts[kScaleCount] = {};

  Scale currentScale = Scale::Major;
  int currentRoot = 0; // 0 = C
};
