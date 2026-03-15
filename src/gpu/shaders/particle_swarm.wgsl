// Particle Swarm -- 2D compute shader (AlgoNebula)
// Agent-based: particles have position + velocity, leave energy trails.
// Two passes: moveParticles (agents), decayTrails (grid).
// This shader handles BOTH in one dispatch using particle indices.
//
// Storage layout:
//   stateIn/stateOut: trail energy grid (f32, width * height)
//   particles: array of { posX, posY, velX, velY } as f32 quads
// Dispatch 1: trail decay (workgroup over grid)
// Dispatch 2: particle move (workgroup over particles)

struct Params {
  width: u32,
  height: u32,
  numParticles: u32,
  _pad: u32,
  trailDecay: f32,     // per-step decay factor (e.g. 0.95)
  socialWeight: f32,   // pull toward global best (e.g. 0.3)
  inertia: f32,        // velocity damping (e.g. 0.9)
  trailDeposit: f32,   // energy added per particle visit (e.g. 0.5)
};

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;
@group(0) @binding(3) var<storage, read_write> particles: array<f32>;

// Simple hash for pseudo-random (deterministic per invocation)
fn hash(seed: u32) -> f32 {
  var s = seed;
  s ^= s >> 16u;
  s *= 0x45d9f3bu;
  s ^= s >> 16u;
  s *= 0x45d9f3bu;
  s ^= s >> 16u;
  return f32(s) / f32(0xFFFFFFFFu);
}

fn idx(r: u32, c: u32) -> u32 {
  return r * params.width + c;
}

// Trail decay pass: run over entire grid
@compute @workgroup_size(16, 16)
fn decayTrails(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x;
  let row = gid.y;

  if (col >= params.width || row >= params.height) {
    return;
  }

  // Dummy read to ensure binding 3 is in auto-layout for shared bind groups
  _ = particles[0];

  let i = idx(row, col);
  stateOut[i] = stateIn[i] * params.trailDecay;
}

// Particle move pass: run over particles
@compute @workgroup_size(64)
fn moveParticles(@builtin(global_invocation_id) gid: vec3<u32>) {
  let pid = gid.x;
  if (pid >= params.numParticles) {
    return;
  }

  // Dummy read to ensure binding 1 is in auto-layout for shared bind groups
  _ = stateIn[0];

  let base = pid * 4u;
  var px = particles[base];
  var py = particles[base + 1u];
  var vx = particles[base + 2u];
  var vy = particles[base + 3u];

  let w = f32(params.width);
  let h = f32(params.height);

  // Random jitter (seeded by particle ID + position)
  let seed = pid * 1337u + u32(px * 100.0) + u32(py * 7919.0);
  let rx = (hash(seed) - 0.5) * 2.0;
  let ry = (hash(seed + 1u) - 0.5) * 2.0;

  // Sample local gradient (crude: compare left/right, up/down energy)
  let cx = u32(px) % params.width;
  let cy = u32(py) % params.height;
  let left  = stateOut[idx(cy, (cx + params.width - 1u) % params.width)];
  let right = stateOut[idx(cy, (cx + 1u) % params.width)];
  let up    = stateOut[idx((cy + params.height - 1u) % params.height, cx)];
  let down  = stateOut[idx((cy + 1u) % params.height, cx)];
  let gradX = right - left;
  let gradY = down - up;

  // Update velocity: inertia + social pull toward gradient + random
  vx = params.inertia * vx + params.socialWeight * gradX + 0.1 * rx;
  vy = params.inertia * vy + params.socialWeight * gradY + 0.1 * ry;

  // Clamp velocity
  let speed = sqrt(vx * vx + vy * vy);
  if (speed > 2.0) {
    vx = vx / speed * 2.0;
    vy = vy / speed * 2.0;
  }

  // Move (toroidal)
  px = (px + vx + w) % w;
  py = (py + vy + h) % h;

  // Write back
  particles[base]      = px;
  particles[base + 1u] = py;
  particles[base + 2u] = vx;
  particles[base + 3u] = vy;

  // Deposit trail energy
  let depX = u32(px) % params.width;
  let depY = u32(py) % params.height;
  // Atomic add not available for f32 in WGSL, use non-atomic (acceptable
  // for visual trail -- minor race is OK)
  stateOut[idx(depY, depX)] = stateOut[idx(depY, depX)] + params.trailDeposit;
}
