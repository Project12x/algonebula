// Lenia 2D -- Continuous-neighborhood CA compute shader (AlgoNebula)
// Bell-curve kernel convolution + Gaussian growth function.
// Uses direct convolution (adequate for small kernels, radius <= 5).
// For large grids with large kernels, FFT would be better but
// direct is simpler and sufficient for typical radii (3-5).

struct Params {
  width: u32,
  height: u32,
  radius: u32,       // kernel radius (e.g. 3)
  _pad0: u32,
  growthMu: f32,     // growth center (e.g. 0.15)
  growthSigma: f32,  // growth width (e.g. 0.045)
  dt: f32,           // time step (e.g. 0.1)
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

// Bell-curve kernel weight: exp(-0.5 * ((d/r - 0.5) / 0.15)^2)
fn kernelWeight(dist: f32, radius: f32) -> f32 {
  let normalized = dist / radius;
  let x = (normalized - 0.5) / 0.15;
  return exp(-0.5 * x * x);
}

// Gaussian growth function
fn growth(potential: f32, mu: f32, sigma: f32) -> f32 {
  let x = (potential - mu) / sigma;
  return 2.0 * exp(-0.5 * x * x) - 1.0;
}

@compute @workgroup_size(16, 16)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
  let col = gid.x;
  let row = gid.y;

  if (col >= params.width || row >= params.height) {
    return;
  }

  let r = i32(params.radius);
  let rf = f32(params.radius);
  var potential: f32 = 0.0;
  var kernelSum: f32 = 0.0;

  // Convolution with bell-curve kernel
  for (var dr: i32 = -r; dr <= r; dr++) {
    for (var dc: i32 = -r; dc <= r; dc++) {
      let dist = sqrt(f32(dr * dr + dc * dc));
      if (dist > rf) {
        continue;
      }
      if (dr == 0 && dc == 0) {
        continue;
      }
      let w = kernelWeight(dist, rf);
      let nr = wrap(i32(row) + dr, params.height);
      let nc = wrap(i32(col) + dc, params.width);
      potential += stateIn[idx(nr, nc)] * w;
      kernelSum += w;
    }
  }

  // Normalize
  if (kernelSum > 0.0) {
    potential /= kernelSum;
  }

  // Apply growth and integrate
  let current = stateIn[idx(row, col)];
  let g = growth(potential, params.growthMu, params.growthSigma);
  let next = clamp(current + params.dt * g, 0.0, 1.0);

  stateOut[idx(row, col)] = next;
}
