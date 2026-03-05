// Brownian Field -- 2D compute shader (AlgoNebula)
// Random-walk particles deposit energy into a grid.
// Grid energy diffuses and decays each step.
// Two passes: diffuseDecay (grid), walkDeposit (particles).

struct Params {
  width: u32,
  height: u32,
  numWalkers: u32,
  stepCount: u32,     // monotonic frame counter (for RNG seeding)
  diffusionRate: f32,  // fraction of energy that spreads (e.g. 0.1)
  decayRate: f32,      // per-step energy decay (e.g. 0.98)
  deposit: f32,        // energy deposited per walker visit (e.g. 0.3)
  _pad: u32,
};

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;
@group(0) @binding(3) var<storage, read_write> walkers: array<f32>;

fn hash(seed: u32) -> u32 {
  var s = seed;
  s ^= s >> 16u;
  s *= 0x45d9f3bu;
  s ^= s >> 16u;
  s *= 0x45d9f3bu;
  s ^= s >> 16u;
  return s;
}

fn idx(r: u32, c: u32) -> u32 {
  return r * params.width + c;
}

fn wrap(val: i32, max: u32) -> u32 {
  return u32((val + i32(max)) % i32(max));
}

// Grid pass: diffuse + decay
@compute @workgroup_size(16, 16)
fn diffuseDecay(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x;
  let row = gid.y;

  if (col >= params.width || row >= params.height) {
    return;
  }

  let center = stateIn[idx(row, col)];

  // Average of 4 cardinal neighbors (for diffusion)
  let up    = stateIn[idx(wrap(i32(row) - 1, params.height), col)];
  let down  = stateIn[idx(wrap(i32(row) + 1, params.height), col)];
  let left  = stateIn[idx(row, wrap(i32(col) - 1, params.width))];
  let right = stateIn[idx(row, wrap(i32(col) + 1, params.width))];
  let avg = (up + down + left + right) * 0.25;

  // Blend toward neighbors (diffusion) + decay
  let diffused = mix(center, avg, params.diffusionRate);
  stateOut[idx(row, col)] = diffused * params.decayRate;
}

// Walker pass: random walk + deposit
@compute @workgroup_size(64)
fn walkDeposit(@builtin(global_invocation_id) gid: vec3<u32>) {
  let wid = gid.x;
  if (wid >= params.numWalkers) {
    return;
  }

  let base = wid * 2u;
  var wx = walkers[base];
  var wy = walkers[base + 1u];

  // Random walk: hash-based direction
  let seed = wid * 7919u + params.stepCount * 1337u;
  let h = hash(seed);
  let dir = h % 4u; // 0=up, 1=down, 2=left, 3=right

  let w = f32(params.width);
  let h2 = f32(params.height);

  switch dir {
    case 0u: { wy = (wy - 1.0 + h2) % h2; }
    case 1u: { wy = (wy + 1.0) % h2; }
    case 2u: { wx = (wx - 1.0 + w) % w; }
    default: { wx = (wx + 1.0) % w; }
  }

  // Occasional diagonal step for variety
  let h3 = hash(seed + 42u);
  if (h3 % 8u == 0u) {
    let diagDir = (h3 / 8u) % 4u;
    switch diagDir {
      case 0u: { wx = (wx - 1.0 + w) % w; wy = (wy - 1.0 + h2) % h2; }
      case 1u: { wx = (wx + 1.0) % w; wy = (wy - 1.0 + h2) % h2; }
      case 2u: { wx = (wx - 1.0 + w) % w; wy = (wy + 1.0) % h2; }
      default: { wx = (wx + 1.0) % w; wy = (wy + 1.0) % h2; }
    }
  }

  walkers[base] = wx;
  walkers[base + 1u] = wy;

  // Deposit energy
  let depX = u32(wx) % params.width;
  let depY = u32(wy) % params.height;
  stateOut[idx(depY, depX)] = stateOut[idx(depY, depX)] + params.deposit;
}
