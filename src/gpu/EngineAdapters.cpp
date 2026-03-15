#include "EngineAdapters.h"

// Embed WGSL shader sources as raw string literals.
// These match the .wgsl files in src/gpu/shaders/ but are compiled
// directly into the binary for zero-dependency deployment.

namespace algonebula {

std::string GoLCompute::getShaderSource() const {
  return R"(
struct Params {
  width: u32, height: u32, birthMask: u32, survivalMask: u32,
};
@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;

fn idx(r: u32, c: u32) -> u32 { return r * params.width + c; }
fn wrap(val: i32, max: u32) -> u32 { return u32((val + i32(max)) % i32(max)); }

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x; let row = gid.y;
  if (col >= params.width || row >= params.height) { return; }
  var neighbors: u32 = 0u;
  for (var dr: i32 = -1; dr <= 1; dr++) {
    for (var dc: i32 = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0) { continue; }
      let nr = wrap(i32(row) + dr, params.height);
      let nc = wrap(i32(col) + dc, params.width);
      if (stateIn[idx(nr, nc)] > 0.5) { neighbors++; }
    }
  }
  let alive = stateIn[idx(row, col)] > 0.5;
  let mask = 1u << neighbors;
  var next: f32 = 0.0;
  if (alive && (params.survivalMask & mask) != 0u) { next = 1.0; }
  else if (!alive && (params.birthMask & mask) != 0u) { next = 1.0; }
  stateOut[idx(row, col)] = next;
}
)";
}

std::string BriansBrainCompute::getShaderSource() const {
  return R"(
struct Params { width: u32, height: u32, _pad0: u32, _pad1: u32, };
@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;
fn idx(r: u32, c: u32) -> u32 { return r * params.width + c; }
fn wrap(val: i32, max: u32) -> u32 { return u32((val + i32(max)) % i32(max)); }

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x; let row = gid.y;
  if (col >= params.width || row >= params.height) { return; }
  let current = stateIn[idx(row, col)];
  if (current > 0.75) { stateOut[idx(row, col)] = 0.5; return; }
  if (current > 0.25 && current < 0.75) { stateOut[idx(row, col)] = 0.0; return; }
  var count: u32 = 0u;
  for (var dr: i32 = -1; dr <= 1; dr++) {
    for (var dc: i32 = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0) { continue; }
      if (stateIn[idx(wrap(i32(row)+dr, params.height), wrap(i32(col)+dc, params.width))] > 0.75) { count++; }
    }
  }
  stateOut[idx(row, col)] = select(0.0, 1.0, count == 2u);
}
)";
}

std::string CyclicCACompute::getShaderSource() const {
  return R"(
struct Params { width: u32, height: u32, numStates: u32, threshold: u32, };
@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;
fn idx(r: u32, c: u32) -> u32 { return r * params.width + c; }
fn wrap(val: i32, max: u32) -> u32 { return u32((val + i32(max)) % i32(max)); }

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x; let row = gid.y;
  if (col >= params.width || row >= params.height) { return; }
  let current = stateIn[idx(row, col)];
  let stateIdx = u32(current * f32(params.numStates - 1u) + 0.5);
  let nextState = (stateIdx + 1u) % params.numStates;
  var count: u32 = 0u;
  for (var dr: i32 = -1; dr <= 1; dr++) {
    for (var dc: i32 = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0) { continue; }
      let nv = stateIn[idx(wrap(i32(row)+dr, params.height), wrap(i32(col)+dc, params.width))];
      if (u32(nv * f32(params.numStates - 1u) + 0.5) == nextState) { count++; }
    }
  }
  stateOut[idx(row, col)] = select(current, f32(nextState) / f32(params.numStates - 1u), count >= params.threshold);
}
)";
}

std::string ReactionDiffusionCompute::getShaderSource() const {
  return R"(
struct Params {
  width: u32, height: u32, feed: f32, kill: f32,
  diffU: f32, diffV: f32, dt: f32, _pad: u32,
};
@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;
fn idx(r: u32, c: u32) -> u32 { return (r * params.width + c) * 2u; }
fn wrap(val: i32, max: u32) -> u32 { return u32((val + i32(max)) % i32(max)); }

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x; let row = gid.y;
  if (col >= params.width || row >= params.height) { return; }
  let i = idx(row, col);
  let u = stateIn[i]; let v = stateIn[i + 1u];
  let up = idx(wrap(i32(row)-1, params.height), col);
  let dn = idx(wrap(i32(row)+1, params.height), col);
  let lt = idx(row, wrap(i32(col)-1, params.width));
  let rt = idx(row, wrap(i32(col)+1, params.width));
  let lapU = stateIn[up] + stateIn[dn] + stateIn[lt] + stateIn[rt] - 4.0*u;
  let lapV = stateIn[up+1u] + stateIn[dn+1u] + stateIn[lt+1u] + stateIn[rt+1u] - 4.0*v;
  let uvv = u * v * v;
  stateOut[i]    = clamp(u + params.dt*(params.diffU*lapU - uvv + params.feed*(1.0-u)), 0.0, 1.0);
  stateOut[i+1u] = clamp(v + params.dt*(params.diffV*lapV + uvv - (params.feed+params.kill)*v), 0.0, 1.0);
}
)";
}

std::string Lenia2DCompute::getShaderSource() const {
  return R"(
struct Params {
  width: u32, height: u32, radius: u32, _pad0: u32,
  growthMu: f32, growthSigma: f32, dt: f32, _pad1: u32,
};
@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;
fn idx(r: u32, c: u32) -> u32 { return r * params.width + c; }
fn wrap(val: i32, max: u32) -> u32 { return u32((val + i32(max)) % i32(max)); }
fn kernelW(dist: f32, r: f32) -> f32 { let x = (dist/r - 0.5)/0.15; return exp(-0.5*x*x); }
fn growth(pot: f32, mu: f32, sig: f32) -> f32 { let x = (pot-mu)/sig; return 2.0*exp(-0.5*x*x) - 1.0; }

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x; let row = gid.y;
  if (col >= params.width || row >= params.height) { return; }
  let r = i32(params.radius); let rf = f32(params.radius);
  var pot: f32 = 0.0; var ks: f32 = 0.0;
  for (var dr: i32 = -r; dr <= r; dr++) {
    for (var dc: i32 = -r; dc <= r; dc++) {
      let d = sqrt(f32(dr*dr + dc*dc));
      if (d > rf || (dr==0 && dc==0)) { continue; }
      let w = kernelW(d, rf);
      pot += stateIn[idx(wrap(i32(row)+dr, params.height), wrap(i32(col)+dc, params.width))] * w;
      ks += w;
    }
  }
  if (ks > 0.0) { pot /= ks; }
  let cur = stateIn[idx(row, col)];
  stateOut[idx(row, col)] = clamp(cur + params.dt * growth(pot, params.growthMu, params.growthSigma), 0.0, 1.0);
}
)";
}

std::string ParticleSwarmCompute::getShaderSource() const {
  return R"(
struct Params {
  width: u32, height: u32, numParticles: u32, _pad: u32,
  trailDecay: f32, socialWeight: f32, inertia: f32, trailDeposit: f32,
};
@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;
@group(0) @binding(3) var<storage, read_write> particles: array<f32>;
fn idx(r: u32, c: u32) -> u32 { return r * params.width + c; }
fn hash(seed: u32) -> f32 {
  var s = seed; s ^= s >> 16u; s *= 0x45d9f3bu; s ^= s >> 16u;
  s *= 0x45d9f3bu; s ^= s >> 16u; return f32(s) / f32(0xFFFFFFFFu);
}

@compute @workgroup_size(16, 16)
fn decayTrails(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x; let row = gid.y;
  if (col >= params.width || row >= params.height) { return; }
  _ = particles[0];
  stateOut[idx(row, col)] = stateIn[idx(row, col)] * params.trailDecay;
}

@compute @workgroup_size(64)
fn moveParticles(@builtin(global_invocation_id) gid: vec3<u32>) {
  let pid = gid.x;
  if (pid >= params.numParticles) { return; }
  _ = stateIn[0];
  let base = pid * 4u;
  var px = particles[base];
  var py = particles[base + 1u];
  var vx = particles[base + 2u];
  var vy = particles[base + 3u];
  let w = f32(params.width);
  let h = f32(params.height);
  let seed = pid * 1337u + u32(px * 100.0) + u32(py * 7919.0);
  let rx = (hash(seed) - 0.5) * 2.0;
  let ry = (hash(seed + 1u) - 0.5) * 2.0;
  let cx = u32(px) % params.width;
  let cy = u32(py) % params.height;
  let left  = stateOut[idx(cy, (cx + params.width - 1u) % params.width)];
  let right = stateOut[idx(cy, (cx + 1u) % params.width)];
  let up    = stateOut[idx((cy + params.height - 1u) % params.height, cx)];
  let down  = stateOut[idx((cy + 1u) % params.height, cx)];
  let gradX = right - left;
  let gradY = down - up;
  vx = params.inertia * vx + params.socialWeight * gradX + 0.1 * rx;
  vy = params.inertia * vy + params.socialWeight * gradY + 0.1 * ry;
  let speed = sqrt(vx * vx + vy * vy);
  if (speed > 2.0) { vx = vx / speed * 2.0; vy = vy / speed * 2.0; }
  px = (px + vx + w) % w;
  py = (py + vy + h) % h;
  particles[base]      = px;
  particles[base + 1u] = py;
  particles[base + 2u] = vx;
  particles[base + 3u] = vy;
  let depX = u32(px) % params.width;
  let depY = u32(py) % params.height;
  stateOut[idx(depY, depX)] = stateOut[idx(depY, depX)] + params.trailDeposit;
}
)";
}

std::string BrownianFieldCompute::getShaderSource() const {
  return R"(
struct Params {
  width: u32, height: u32, numWalkers: u32, stepCount: u32,
  diffusionRate: f32, decayRate: f32, deposit: f32, _pad: u32,
};
@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;
@group(0) @binding(3) var<storage, read_write> walkers: array<f32>;
fn idx(r: u32, c: u32) -> u32 { return r * params.width + c; }
fn wrap(val: i32, max: u32) -> u32 { return u32((val + i32(max)) % i32(max)); }
fn hash(seed: u32) -> u32 {
  var s = seed; s ^= s >> 16u; s *= 0x45d9f3bu; s ^= s >> 16u;
  s *= 0x45d9f3bu; s ^= s >> 16u; return s;
}

@compute @workgroup_size(16, 16)
fn diffuseDecay(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x; let row = gid.y;
  if (col >= params.width || row >= params.height) { return; }
  _ = walkers[0];
  let center = stateIn[idx(row, col)];
  let up = stateIn[idx(wrap(i32(row)-1, params.height), col)];
  let dn = stateIn[idx(wrap(i32(row)+1, params.height), col)];
  let lt = stateIn[idx(row, wrap(i32(col)-1, params.width))];
  let rt = stateIn[idx(row, wrap(i32(col)+1, params.width))];
  stateOut[idx(row, col)] = mix(center, (up+dn+lt+rt)*0.25, params.diffusionRate) * params.decayRate;
}

@compute @workgroup_size(64)
fn walkDeposit(@builtin(global_invocation_id) gid: vec3<u32>) {
  let wid = gid.x;
  if (wid >= params.numWalkers) { return; }
  _ = stateIn[0];
  let base = wid * 2u;
  var wx = walkers[base];
  var wy = walkers[base + 1u];
  let seed = wid * 7919u + params.stepCount * 1337u;
  let h = hash(seed);
  let dir = h % 4u;
  let w = f32(params.width);
  let h2 = f32(params.height);
  switch dir {
    case 0u: { wy = (wy - 1.0 + h2) % h2; }
    case 1u: { wy = (wy + 1.0) % h2; }
    case 2u: { wx = (wx - 1.0 + w) % w; }
    default: { wx = (wx + 1.0) % w; }
  }
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
  let depX = u32(wx) % params.width;
  let depY = u32(wy) % params.height;
  stateOut[idx(depY, depX)] = stateOut[idx(depY, depX)] + params.deposit;
}
)";
}

} // namespace algonebula
