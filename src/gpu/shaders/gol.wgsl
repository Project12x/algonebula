// Game of Life -- 2D compute shader (AlgoNebula)
// Toroidal wrapping, configurable birth/survival bitmasks.
// Input: storage buffer `stateIn` (u32, 0 or 1 per cell)
// Output: storage buffer `stateOut`
// Params: uniform { width, height, birthMask, survivalMask }

struct Params {
  width: u32,
  height: u32,
  birthMask: u32,     // bit N set = birth on N neighbors
  survivalMask: u32,  // bit N set = survive on N neighbors
};

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> stateIn: array<f32>;
@group(0) @binding(2) var<storage, read_write> stateOut: array<f32>;

fn idx(r: u32, c: u32) -> u32 {
  return r * params.width + c;
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

  // Count live neighbors (Moore, toroidal)
  var neighbors: u32 = 0u;
  for (var dr: i32 = -1; dr <= 1; dr++) {
    for (var dc: i32 = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0) {
        continue;
      }
      let nr = wrap(i32(row) + dr, params.height);
      let nc = wrap(i32(col) + dc, params.width);
      if (stateIn[idx(nr, nc)] > 0.5) {
        neighbors++;
      }
    }
  }

  let current = stateIn[idx(row, col)];
  let alive = current > 0.5;
  var next: f32 = 0.0;

  let mask = 1u << neighbors;
  if (alive && (params.survivalMask & mask) != 0u) {
    next = 1.0;
  } else if (!alive && (params.birthMask & mask) != 0u) {
    next = 1.0;
  }

  stateOut[idx(row, col)] = next;
}
