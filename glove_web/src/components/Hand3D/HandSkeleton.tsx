import { useRef, useMemo } from 'react';
import { useFrame } from '@react-three/fiber';
import * as THREE from 'three';
import { useHandAnimation } from '../../hooks/useHandAnimation';
import FingerBone from './FingerBone';
import { HAND_CONNECTIONS, FINGER_GROUPS, NUM_KEYPOINTS, COLORS } from '../../utils/constants';
import type { Keypoint3D } from '../../types';

// ── Joint component: colored sphere ──
function Joint({ position, color, radius = 0.06 }: {
  position: Keypoint3D;
  color: string;
  radius?: number;
}) {
  const meshRef = useRef<THREE.Mesh>(null);

  return (
    <mesh ref={meshRef} position={[position.x, position.y, position.z]}>
      <sphereGeometry args={[radius, 16, 16]} />
      <meshStandardMaterial
        color={color}
        roughness={0.4}
        metalness={0.1}
        emissive={color}
        emissiveIntensity={0.15}
      />
    </mesh>
  );
}

// ── Get color for a keypoint based on finger group ──
function getJointColor(kpIndex: number): string {
  if (kpIndex === 0) return COLORS.wrist;
  for (const group of FINGER_GROUPS) {
    if (group.indices.includes(kpIndex)) return group.color;
  }
  return COLORS.joint;
}

// ── Get color for a bone based on its start keypoint ──
function getBoneColor(startIdx: number, _endIdx: number): string {
  return getJointColor(startIdx);
}

export default function HandSkeleton() {
  const handPose = useHandAnimation();
  const groupRef = useRef<THREE.Group>(null);

  // Quaternion-driven wrist rotation
  useFrame(() => {
    if (groupRef.current) {
      const [w, x, y, z] = handPose.quaternion;
      groupRef.current.quaternion.set(x, y, z, w);
    }
  });

  // Memoize bone connections rendering
  const bones = useMemo(() => {
    return HAND_CONNECTIONS.map(([startIdx, endIdx], i) => {
      const start = handPose.keypoints[startIdx];
      const end = handPose.keypoints[endIdx];
      const color = getBoneColor(startIdx, endIdx);
      return <FingerBone key={`bone-${i}`} start={start} end={end} color={color} />;
    });
  }, [handPose.keypoints]); // eslint-disable-line react-hooks/exhaustive-deps

  // Memoize joint rendering
  const joints = useMemo(() => {
    return Array.from({ length: NUM_KEYPOINTS }, (_, i) => {
      const kp = handPose.keypoints[i];
      const color = getJointColor(i);
      const isWrist = i === 0;
      return (
        <Joint
          key={`joint-${i}`}
          position={kp}
          color={color}
          radius={isWrist ? 0.1 : 0.05}
        />
      );
    });
  }, [handPose.keypoints]);

  return (
    <group ref={groupRef} position={[0, 0.5, 0]}>
      {/* Palm center indicator */}
      <mesh position={[0, 0.9, 0]}>
        <sphereGeometry args={[0.03, 8, 8]} />
        <meshBasicMaterial color={COLORS.skin} transparent opacity={0.5} />
      </mesh>

      {/* Bones */}
      {bones}

      {/* Joints */}
      {joints}
    </group>
  );
}
