// Reaction-Diffusion (Gray-Scott) -- 2D compute shader (AlgoNebula)
// Two-channel (u, v) simulation with diffusion + reaction + feed/kill.
// Uses 5-point Laplacian stencil. Toroidal wrapping.
// Storage: interleaved [u0, v0, u1, v1, ...] as f32.

struct Params {
  width: u32,
  height: u32,
  feed: f32,       // feed rate (F), typically 0.01-0.08
  kill: f32,       // kill rate (k), typically 0.03-0.07
  diffU: f32,      // diffusion rate U, typically 0.16-0.21
  diffV: f32,      // diffusion rate V, typically 0.08-0.12
  dt: f32,         // time step, typically 1.0
  _pad: u32,
};

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;

fn idx(r: u32, c: u32) -> u32 {
  return (r * params.width + c) * 2u; // *2 for interleaved u,v
}

fn wrap(val: i32, max: u32) -> u32 {
  return u32((val + i32(max)) % i32(max));
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x;
  let row = gid.y;

  if (col >= params.width || row >= params.height) {
    return;
  }

  let i = idx(row, col);
  let u = stateIn[i];
  let v = stateIn[i + 1u];

  // 5-point Laplacian (toroidal)
  let up    = idx(wrap(i32(row) - 1, params.height), col);
  let down  = idx(wrap(i32(row) + 1, params.height), col);
  let left  = idx(row, wrap(i32(col) - 1, params.width));
  let right = idx(row, wrap(i32(col) + 1, params.width));

  let lapU = stateIn[up] + stateIn[down] + stateIn[left] + stateIn[right] - 4.0 * u;
  let lapV = stateIn[up + 1u] + stateIn[down + 1u] + stateIn[left + 1u] + stateIn[right + 1u] - 4.0 * v;

  // Gray-Scott reaction
  let uvv = u * v * v;
  let newU = u + params.dt * (params.diffU * lapU - uvv + params.feed * (1.0 - u));
  let newV = v + params.dt * (params.diffV * lapV + uvv - (params.feed + params.kill) * v);

  stateOut[i]      = clamp(newU, 0.0, 1.0);
  stateOut[i + 1u] = clamp(newV, 0.0, 1.0);
}
