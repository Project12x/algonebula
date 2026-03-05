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
  stateOut[idx(row, col)] = stateIn[idx(row, col)] * params.trailDecay;
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

@compute @workgroup_size(16, 16)
fn diffuseDecay(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x; let row = gid.y;
  if (col >= params.width || row >= params.height) { return; }
  let center = stateIn[idx(row, col)];
  let up = stateIn[idx(wrap(i32(row)-1, params.height), col)];
  let dn = stateIn[idx(wrap(i32(row)+1, params.height), col)];
  let lt = stateIn[idx(row, wrap(i32(col)-1, params.width))];
  let rt = stateIn[idx(row, wrap(i32(col)+1, params.width))];
  stateOut[idx(row, col)] = mix(center, (up+dn+lt+rt)*0.25, params.diffusionRate) * params.decayRate;
}
)";
}

} // namespace algonebula
