// Brian's Brain -- 2D compute shader (AlgoNebula)
// 3-state automaton: 0=dead, 1=alive, 2=dying
// Rules: dead cell with exactly 2 alive neighbors -> alive
//        alive cell -> dying
//        dying cell -> dead
// Toroidal wrapping.

struct Params {
  width: u32,
  height: u32,
  _pad0: u32,
  _pad1: u32,
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

// State encoding: 0.0 = dead, 1.0 = alive, 0.5 = dying
const DEAD: f32 = 0.0;
const ALIVE: f32 = 1.0;
const DYING: f32 = 0.5;

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x;
  let row = gid.y;

  if (col >= params.width || row >= params.height) {
    return;
  }

  let current = stateIn[idx(row, col)];

  // Alive -> dying
  if (current > 0.75) {
    stateOut[idx(row, col)] = DYING;
    return;
  }

  // Dying -> dead
  if (current > 0.25 && current < 0.75) {
    stateOut[idx(row, col)] = DEAD;
    return;
  }

  // Dead: count alive neighbors
  var aliveNeighbors: u32 = 0u;
  for (var dr: i32 = -1; dr <= 1; dr++) {
    for (var dc: i32 = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0) {
        continue;
      }
      let nr = wrap(i32(row) + dr, params.height);
      let nc = wrap(i32(col) + dc, params.width);
      if (stateIn[idx(nr, nc)] > 0.75) {
        aliveNeighbors++;
      }
    }
  }

  if (aliveNeighbors == 2u) {
    stateOut[idx(row, col)] = ALIVE;
  } else {
    stateOut[idx(row, col)] = DEAD;
  }
}
