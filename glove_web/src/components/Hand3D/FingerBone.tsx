import { useMemo } from 'react';
import * as THREE from 'three';
import type { Keypoint3D } from '../../types';

interface FingerBoneProps {
  start: Keypoint3D;
  end: Keypoint3D;
  color: string;
  radius?: number;
}

export default function FingerBone({
  start,
  end,
  color,
  radius = 0.025,
}: FingerBoneProps) {
  // Calculate position, rotation, and scale for the cylinder
  const { position, quaternion, length } = useMemo(() => {
    const direction = new THREE.Vector3(
      end.x - start.x,
      end.y - start.y,
      end.z - start.z,
    );
    const len = Math.max(0.001, direction.length());
    direction.normalize();

    // Default cylinder is oriented along Y axis
    const defaultDir = new THREE.Vector3(0, 1, 0);

    // Compute quaternion to rotate from default to target direction
    const quat = new THREE.Quaternion();
    quat.setFromUnitVectors(defaultDir, direction);

    // Midpoint between start and end
    const mid = new THREE.Vector3(
      (start.x + end.x) / 2,
      (start.y + end.y) / 2,
      (start.z + end.z) / 2,
    );

    return {
      position: mid,
      quaternion: quat,
      length: len,
    };
  }, [start.x, start.y, start.z, end.x, end.y, end.z]);

  return (
    <mesh position={position} quaternion={quaternion}>
      <cylinderGeometry args={[radius, radius, length, 8, 1]} />
      <meshStandardMaterial
        color={color}
        roughness={0.5}
        metalness={0.2}
        transparent
        opacity={0.85}
      />
    </mesh>
  );
}
