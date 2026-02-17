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

  /// Check if two MIDI notes form a consonant interval.
  /// Consonant: unison(0), m3(3), M3(4), P4(5), P5(7), m6(8), M6(9), octave(12)
  /// Dissonant: m2(1), M2(2), tritone(6), m7(10), M7(11)
  static bool isConsonant(int note1, int note2) {
    int interval = std::abs(note1 - note2) % 12;
    // Lookup table: 1 = consonant, 0 = dissonant
    static constexpr int consonantTable[12] = {1, 0, 0, 1, 1, 1,
                                               0, 1, 1, 1, 0, 0};
    return consonantTable[interval] != 0;
  }

  /// Check if a candidate note is consonant with ALL active notes.
  /// Returns true if activeCount is 0 (no conflicts possible).
  static bool isConsonantWithAll(int candidateNote, const int *activeNotes,
                                 int activeCount) {
    for (int i = 0; i < activeCount; ++i) {
      if (!isConsonant(candidateNote, activeNotes[i]))
        return false;
    }
    return true;
  }

  /// Snap a dissonant note to the nearest consonant pitch.
  /// Searches +-6 semitones from the candidate for a note consonant with ALL
  /// active notes. Returns the original note if no consonant alternative found.
  static int snapToConsonant(int candidateNote, const int *activeNotes,
                             int activeCount) {
    if (activeCount == 0 ||
        isConsonantWithAll(candidateNote, activeNotes, activeCount))
      return candidateNote;

    // Search outward from candidate: +-1, +-2, ... +-6
    for (int offset = 1; offset <= 6; ++offset) {
      int above = candidateNote + offset;
      int below = candidateNote - offset;
      if (above <= 127 && isConsonantWithAll(above, activeNotes, activeCount))
        return above;
      if (below >= 0 && isConsonantWithAll(below, activeNotes, activeCount))
        return below;
    }
    return candidateNote; // No consonant neighbor found — keep original
  }

  /// Quantize with gravity toward chord tones (root/5th/3rd).
  /// @param gravity 0.0 = normal quantize, 1.0 = always snap to chord tone.
  /// @param rng Pointer to RNG state for probabilistic snapping.
  int quantizeWeighted(int cellRow, int cellCol, int baseOctave, int octaveSpan,
                       int gridCols, float gravity, uint64_t &rng) const {
    // Always start with normal quantization
    int normal = quantize(cellRow, cellCol, baseOctave, octaveSpan, gridCols);
    if (gravity <= 0.0f)
      return normal;

    // Roll to decide if we snap to chord tone
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    float roll = static_cast<float>(rng & 0xFFFF) / 65535.0f;
    if (roll >= gravity)
      return normal; // Keep normal pitch

    // Snap to nearest chord tone (root, 3rd, 5th in current scale)
    // Chord tones are scale degrees 0, 2, 4 (root, 3rd, 5th in diatonic)
    const auto &degrees = scaleDegrees[static_cast<int>(currentScale)];
    int degreeCount = scaleDegreeCounts[static_cast<int>(currentScale)];
    if (degreeCount < 3)
      return normal; // Not enough degrees for chord tones

    // Pick chord tone indices based on scale size
    int chordDegreeIndices[3] = {0, 2, 4}; // root, 3rd, 5th in scale
    if (degreeCount == 5) {
      // Pentatonic: root(0), 3rd(2), 5th(3)
      chordDegreeIndices[1] = 2;
      chordDegreeIndices[2] = 3;
    }

    // Find which octave the normal note is in
    int noteInOctave = (normal - currentRoot) % 12;
    if (noteInOctave < 0)
      noteInOctave += 12;
    int octaveBase = normal - noteInOctave;

    // Find nearest chord tone
    int bestNote = normal;
    int bestDist = 999;
    for (int ci = 0; ci < 3; ++ci) {
      int idx = chordDegreeIndices[ci];
      if (idx >= degreeCount)
        continue;
      int chordSemitone = degrees[idx];
      int candidate = octaveBase + chordSemitone;
      // Check this octave and adjacent
      for (int octOff = -1; octOff <= 1; ++octOff) {
        int c = candidate + octOff * 12;
        if (c < 0 || c > 127)
          continue;
        int dist = std::abs(c - normal);
        if (dist < bestDist) {
          bestDist = dist;
          bestNote = c;
        }
      }
    }
    return bestNote;
  }

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
