// ── Quaternion Type ──
export type Quat = [number, number, number, number]; // [w, x, y, z]

// ── Euler Angles → Quaternion ──
// Input: roll (X), pitch (Y), yaw (Z) in radians
export function eulerToQuaternion(roll: number, pitch: number, yaw: number): Quat {
  const cr = Math.cos(roll * 0.5);
  const sr = Math.sin(roll * 0.5);
  const cp = Math.cos(pitch * 0.5);
  const sp = Math.sin(pitch * 0.5);
  const cy = Math.cos(yaw * 0.5);
  const sy = Math.sin(yaw * 0.5);

  const w = cr * cp * cy + sr * sp * sy;
  const x = sr * cp * cy - cr * sp * sy;
  const y = cr * sp * cy + sr * cp * sy;
  const z = cr * cp * sy - sr * sp * cy;

  // Normalize
  const len = Math.sqrt(w * w + x * x + y * y + z * z);
  return len > 0 ? [w / len, x / len, y / len, z / len] : [1, 0, 0, 0];
}

// ── Quaternion → Euler Angles ──
// Returns [roll, pitch, yaw] in radians
export function quaternionToEuler(w: number, x: number, y: number, z: number): [number, number, number] {
  // Roll (x-axis rotation)
  const sinrCosp = 2 * (w * x + y * z);
  const cosrCosp = 1 - 2 * (x * x + y * y);
  const roll = Math.atan2(sinrCosp, cosrCosp);

  // Pitch (y-axis rotation)
  const sinp = 2 * (w * y - z * x);
  const pitch =
    Math.abs(sinp) >= 1
      ? Math.sign(sinp) * (Math.PI / 2)
      : Math.asin(sinp);

  // Yaw (z-axis rotation)
  const sinyCosp = 2 * (w * z + x * y);
  const cosyCosp = 1 - 2 * (y * y + z * z);
  const yaw = Math.atan2(sinyCosp, cosyCosp);

  return [roll, pitch, yaw];
}

// ── Multiply two quaternions: q1 * q2 ──
export function multiplyQuaternion(q1: Quat, q2: Quat): Quat {
  const [w1, x1, y1, z1] = q1;
  const [w2, x2, y2, z2] = q2;

  return [
    w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2,
    w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
    w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
    w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2,
  ];
}

// ── Spherical Linear Interpolation (SLERP) ──
export function slerpQuaternion(q1: Quat, q2: Quat, t: number): Quat {
  // Compute cosine of angle between quaternions
  let dot = q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3];

  // If negative dot, negate one quaternion to take the shorter path
  let q2Adjusted: Quat = q2;
  if (dot < 0) {
    q2Adjusted = [-q2[0], -q2[1], -q2[2], -q2[3]];
    dot = -dot;
  }

  // If quaternions are very close, use linear interpolation
  if (dot > 0.9995) {
    const result: Quat = [
      q1[0] + t * (q2Adjusted[0] - q1[0]),
      q1[1] + t * (q2Adjusted[1] - q1[1]),
      q1[2] + t * (q2Adjusted[2] - q1[2]),
      q1[3] + t * (q2Adjusted[3] - q1[3]),
    ];
    // Normalize
    const len = Math.sqrt(
      result[0] ** 2 + result[1] ** 2 + result[2] ** 2 + result[3] ** 2,
    );
    return len > 0
      ? [result[0] / len, result[1] / len, result[2] / len, result[3] / len]
      : [1, 0, 0, 0];
  }

  // SLERP formula
  const theta0 = Math.acos(dot);
  const theta = theta0 * t;
  const sinTheta = Math.sin(theta0);
  const sinThetaInv = 1 / sinTheta;

  const s0 = Math.cos(theta) * sinThetaInv;
  const s1 = Math.sin(theta) * sinThetaInv;

  return [
    s0 * q1[0] + s1 * q2Adjusted[0],
    s0 * q1[1] + s1 * q2Adjusted[1],
    s0 * q1[2] + s1 * q2Adjusted[2],
    s0 * q1[3] + s1 * q2Adjusted[3],
  ];
}

// ── Linear Interpolation ──
export function lerp(a: number, b: number, t: number): number {
  return a + (b - a) * t;
}

// ── Vector3 Lerp ──
export function lerpVec3(
  ax: number, ay: number, az: number,
  bx: number, by: number, bz: number,
  t: number,
): [number, number, number] {
  return [lerp(ax, bx, t), lerp(ay, by, t), lerp(az, bz, t)];
}
