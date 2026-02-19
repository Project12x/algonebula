#include "LeniaEngine.h"
#include <algorithm>
#include <cmath>

// pocketfft for FFT-based convolution
#include "dsp/pocketfft_hdronly.h"

namespace {
uint64_t xorshift64(uint64_t &state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}
} // namespace

LeniaEngine::LeniaEngine(int r, int c)
    : stateField(Grid::kMaxCells, 0.0f), scratch(Grid::kMaxCells, 0.0f),
      grid(r, c), rows(r), cols(c) {
  precomputeKernel();
}

void LeniaEngine::precomputeKernel() {
  // Bell-curve kernel: weight decreases with distance from center
  kernelSum = 0.0f;
  int idx = 0;
  for (int dr = -kRadius; dr <= kRadius; ++dr) {
    for (int dc = -kRadius; dc <= kRadius; ++dc) {
      float dist = std::sqrt(static_cast<float>(dr * dr + dc * dc));
      float normalized = dist / static_cast<float>(kRadius);
      // Bell curve: exp(-0.5 * ((r - 0.5) / 0.15)^2)
      float diff = normalized - 0.5f;
      kernel[idx] = std::exp(-0.5f * diff * diff / (0.15f * 0.15f));
      kernelSum += kernel[idx];
      ++idx;
    }
  }
  // Invalidate FFT cache when kernel changes
  fftPrepared = false;
}

void LeniaEngine::prepareFFT() {
  // Build the kernel in a zero-padded rows x cols real array,
  // then transform it once. We reuse this every step.
  const size_t N = static_cast<size_t>(rows) * static_cast<size_t>(cols);

  // Contiguous real buffer for the kernel (rows x cols, packed)
  fftReal.assign(N, 0.0f);

  // Place kernel centered at (0,0) with toroidal wrap
  int kidx = 0;
  for (int dr = -kRadius; dr <= kRadius; ++dr) {
    for (int dc = -kRadius; dc <= kRadius; ++dc) {
      int wr = ((dr % rows) + rows) % rows;
      int wc = ((dc % cols) + cols) % cols;
      fftReal[static_cast<size_t>(wr) * static_cast<size_t>(cols) +
              static_cast<size_t>(wc)] += kernel[kidx] / kernelSum;
      ++kidx;
    }
  }

  // Forward FFT of kernel (real-to-complex, 2D)
  pocketfft::shape_t shape = {static_cast<size_t>(rows),
                              static_cast<size_t>(cols)};
  pocketfft::stride_t strideIn = {static_cast<ptrdiff_t>(cols * sizeof(float)),
                                  static_cast<ptrdiff_t>(sizeof(float))};
  // Complex output: last axis is cols/2+1 (hermitian symmetry)
  size_t cCols = static_cast<size_t>(cols / 2 + 1);
  pocketfft::stride_t strideOut = {
      static_cast<ptrdiff_t>(cCols * sizeof(std::complex<float>)),
      static_cast<ptrdiff_t>(sizeof(std::complex<float>))};

  kernelFFT.resize(static_cast<size_t>(rows) * cCols);

  pocketfft::r2c(shape, strideIn, strideOut, {0, 1}, pocketfft::FORWARD,
                 fftReal.data(), kernelFFT.data(), 1.0f);

  // Pre-allocate field FFT buffer
  fieldFFT.resize(static_cast<size_t>(rows) * cCols);

  fftPrepared = true;
}

void LeniaEngine::step() {
  if (rows * cols >= kFFTThreshold) {
    stepFFT();
  } else {
    stepDirect();
  }
  projectToGrid();
  ++generation;
}

void LeniaEngine::stepDirect() {
  // Original direct convolution (fast for small grids)
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float neighborSum = 0.0f;
      int kidx = 0;
      for (int dr = -kRadius; dr <= kRadius; ++dr) {
        for (int dc = -kRadius; dc <= kRadius; ++dc) {
          int nr = ((r + dr) % rows + rows) % rows;
          int nc = ((c + dc) % cols + cols) % cols;
          neighborSum += stateField[nr * Grid::kMaxCols + nc] * kernel[kidx];
          ++kidx;
        }
      }

      // Normalize by kernel sum
      float potential = (kernelSum > 0.0f) ? neighborSum / kernelSum : 0.0f;

      // Growth function: Gaussian centered at mu, width sigma
      float diff = potential - kMu;
      float growth =
          2.0f * std::exp(-0.5f * diff * diff / (kSigma * kSigma)) - 1.0f;

      // Update state
      int idx = r * Grid::kMaxCols + c;
      scratch[idx] = stateField[idx] + kDt * growth;

      // Clamp to [0, 1]
      if (scratch[idx] < 0.0f)
        scratch[idx] = 0.0f;
      if (scratch[idx] > 1.0f)
        scratch[idx] = 1.0f;
    }
  }

  std::copy(scratch.begin(), scratch.end(), stateField.begin());
}

void LeniaEngine::stepFFT() {
  if (!fftPrepared)
    prepareFFT();

  const size_t N = static_cast<size_t>(rows) * static_cast<size_t>(cols);
  const size_t cCols = static_cast<size_t>(cols / 2 + 1);

  // 1) Pack state field into contiguous real buffer (rows x cols, no kMaxCols
  // stride)
  fftReal.resize(N);
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      fftReal[static_cast<size_t>(r) * static_cast<size_t>(cols) +
              static_cast<size_t>(c)] = stateField[r * Grid::kMaxCols + c];
    }
  }

  // 2) Forward FFT of state field
  pocketfft::shape_t shape = {static_cast<size_t>(rows),
                              static_cast<size_t>(cols)};
  pocketfft::stride_t strideIn = {static_cast<ptrdiff_t>(cols * sizeof(float)),
                                  static_cast<ptrdiff_t>(sizeof(float))};
  pocketfft::stride_t strideOut = {
      static_cast<ptrdiff_t>(cCols * sizeof(std::complex<float>)),
      static_cast<ptrdiff_t>(sizeof(std::complex<float>))};

  fieldFFT.resize(static_cast<size_t>(rows) * cCols);
  pocketfft::r2c(shape, strideIn, strideOut, {0, 1}, pocketfft::FORWARD,
                 fftReal.data(), fieldFFT.data(), 1.0f);

  // 3) Pointwise multiply in frequency domain
  for (size_t i = 0; i < fieldFFT.size(); ++i) {
    fieldFFT[i] *= kernelFFT[i];
  }

  // 4) Inverse FFT back to spatial domain
  pocketfft::c2r(shape, strideOut, strideIn, {0, 1}, pocketfft::BACKWARD,
                 fieldFFT.data(), fftReal.data(), 1.0f / static_cast<float>(N));

  // 5) Apply growth function and update state
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float potential =
          fftReal[static_cast<size_t>(r) * static_cast<size_t>(cols) +
                  static_cast<size_t>(c)];

      // Growth function: Gaussian centered at mu, width sigma
      float diff = potential - kMu;
      float growth =
          2.0f * std::exp(-0.5f * diff * diff / (kSigma * kSigma)) - 1.0f;

      int idx = r * Grid::kMaxCols + c;
      float newVal = stateField[idx] + kDt * growth;

      // Clamp to [0, 1]
      if (newVal < 0.0f)
        newVal = 0.0f;
      if (newVal > 1.0f)
        newVal = 1.0f;

      scratch[idx] = newVal;
    }
  }

  std::copy(scratch.begin(), scratch.end(), stateField.begin());
}

void LeniaEngine::projectToGrid() {
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      int idx = r * Grid::kMaxCols + c;
      float s = stateField[idx];
      if (s > kThreshold) {
        grid.setCell(r, c, 1);
        grid.setAge(r, c, static_cast<uint16_t>(s * 255.0f));
      } else {
        grid.setCell(r, c, 0);
        grid.setAge(r, c, 0);
      }
    }
  }
}

void LeniaEngine::randomize(uint64_t seed, float density) {
  generation = 0;
  uint64_t state = seed ? seed : 1;
  fftPrepared = false; // Grid dimensions may have changed

  std::fill(stateField.begin(), stateField.end(), 0.0f);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      float val = static_cast<float>(xorshift64(state) >> 32) / 4294967296.0f;
      if (val < density) {
        stateField[r * Grid::kMaxCols + c] = 0.5f + 0.5f * val;
      }
    }
  }

  projectToGrid();
}

void LeniaEngine::randomizeSymmetric(uint64_t seed, float density) {
  generation = 0;
  uint64_t state = seed ? seed : 1;
  fftPrepared = false;
  std::fill(stateField.begin(), stateField.end(), 0.0f);

  const int halfR = (rows + 1) / 2;
  const int halfC = (cols + 1) / 2;

  for (int r = 0; r < halfR; ++r) {
    for (int c = 0; c < halfC; ++c) {
      float val = static_cast<float>(xorshift64(state) >> 32) / 4294967296.0f;
      if (val < density) {
        float v = 0.5f + 0.5f * val;
        int mr = rows - 1 - r;
        int mc = cols - 1 - c;
        stateField[r * Grid::kMaxCols + c] = v;
        stateField[r * Grid::kMaxCols + mc] = v;
        stateField[mr * Grid::kMaxCols + c] = v;
        stateField[mr * Grid::kMaxCols + mc] = v;
      }
    }
  }

  projectToGrid();
}

void LeniaEngine::clear() {
  generation = 0;
  fftPrepared = false;
  std::fill(stateField.begin(), stateField.end(), 0.0f);
  grid.clear();
}
