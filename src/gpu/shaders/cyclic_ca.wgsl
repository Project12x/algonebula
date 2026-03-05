// Cyclic CA -- 2D compute shader (AlgoNebula)
// N-state cyclic automaton with configurable threshold.
// A cell advances to state (s+1) % numStates if it has >= threshold
// neighbors already in that next state. Toroidal wrapping.

struct Params {
  width: u32,
  height: u32,
  numStates: u32,
  threshold: u32,
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

  let current = stateIn[idx(row, col)];
  // Decode float to state index: state = round(current * (numStates - 1))
  let stateIdx = u32(current * f32(params.numStates - 1u) + 0.5);
  let nextState = (stateIdx + 1u) % params.numStates;

  // Count neighbors in the next state (Moore neighborhood)
  var count: u32 = 0u;
  for (var dr: i32 = -1; dr <= 1; dr++) {
    for (var dc: i32 = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0) {
        continue;
      }
      let nr = wrap(i32(row) + dr, params.height);
      let nc = wrap(i32(col) + dc, params.width);
      let neighborVal = stateIn[idx(nr, nc)];
      let neighborState = u32(neighborVal * f32(params.numStates - 1u) + 0.5);
      if (neighborState == nextState) {
        count++;
      }
    }
  }

  if (count >= params.threshold) {
    // Advance to next state
    stateOut[idx(row, col)] = f32(nextState) / f32(params.numStates - 1u);
  } else {
    stateOut[idx(row, col)] = current;
  }
}
