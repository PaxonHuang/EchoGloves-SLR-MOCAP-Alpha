import { Canvas } from '@react-three/fiber';
import { OrbitControls, Grid, PerspectiveCamera } from '@react-three/drei';
import HandSkeleton from './HandSkeleton';
import { COLORS } from '../../utils/constants';

export function HandCanvas() {
  return (
    <div className="h-full w-full">
      <Canvas
        gl={{ antialias: true, alpha: false }}
        dpr={[1, 2]}
        style={{ background: COLORS.background }}
      >
        {/* Camera */}
        <PerspectiveCamera makeDefault position={[0, 3, 6]} fov={50} />

        {/* Lighting */}
        <ambientLight intensity={0.6} />
        <directionalLight position={[5, 8, 5]} intensity={0.8} castShadow />
        <directionalLight position={[-3, 4, -3]} intensity={0.3} />

        {/* Ground Grid */}
        <Grid
          args={[10, 10]}
          cellSize={0.5}
          cellThickness={0.5}
          cellColor="#1e293b"
          sectionSize={2}
          sectionThickness={1}
          sectionColor="#334155"
          fadeDistance={15}
          position={[0, -0.5, 0]}
          rotation={[0, 0, 0]}
          infiniteGrid
        />

        {/* Hand Skeleton */}
        <HandSkeleton />

        {/* Orbit Controls */}
        <OrbitControls
          enablePan={false}
          enableZoom={true}
          minDistance={3}
          maxDistance={12}
          target={[0, 1.5, 0]}
        />
      </Canvas>
    </div>
  );
}

export default HandCanvas;
