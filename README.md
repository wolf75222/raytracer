# Raytracer CPU / GPU / Hybride

Path tracer en C++ et CUDA. Rendu progressif temps réel avec SDL3.

## Build

```bash
# CPU + GPU + Realtime
cmake -B build -DENABLE_TESTS=ON -DENABLE_CUDA=ON -DENABLE_REALTIME=ON
cmake --build build --config Release

# CPU uniquement
cmake -B build -DENABLE_TESTS=ON
cmake --build build --config Release

# Tests
cd build && ctest --build-config Release
```

Prérequis : CMake 3.20+, C++17, CUDA Toolkit (optionnel), SDL3 (téléchargé automatiquement).

## Usage

```bash
# Rendu image
raytracer --gpu --scene showcase --width 800 --spp 200 --output render.ppm
raytracer --scene final --width 800 --spp 100 --output render.ppm
raytracer --gpu --denoise-gpu --scene minimal --spp 50 --output denoised.ppm

# Temps réel (WASD + souris, S = screenshot, ESC = quitter)
raytracer --realtime --scene showcase --width 640 --spp 500

# Hybride CPU+GPU
raytracer --hybrid --gpu-frac 0.7 --scene final --spp 100 --output render.ppm
```

## Scènes

| Scène | Objets | Description |
|-------|--------|-------------|
| `three_spheres` | 5 | Terre, eau (bulle), chrome sur sol gris |
| `final` | ~170 | Anneaux concentriques, verre central, or, rouge |
| `triangle` | 9 | Pyramide, cylindre, verre (CPU uniquement) |
| `cornell` | 14 | Boîte fermée, lumières émissives (CPU uniquement) |
| `showcase` | 16 | Damier, miroir, or, diamant, area light |
| `hall` | ~30 | Colonnes chrome, rubis, émeraude |
| `galaxy` | ~1000 | Spirale 4 bras, stress test GPU |
| `minimal` | 7 | Noir/blanc/miroir, DOF prononcé |

## Architecture

```
src/
  math/       vec3, ray, interval, aabb, random (.cuh pour CUDA)
  scene/      camera, material, primitives, scene, bvh_builder
  cpu/        cpu_renderer (multi-thread par tuiles 16x16)
  gpu/        gpu_renderer, gpu_types, gpu_realtime (CUDA + SDL3)
  hybrid/     hybrid_renderer (split horizontal CPU+GPU)
  utils/      image (PPM), denoise, timer
```

## Performances

RTX 3090 + i7, scène final 800x450, 50 spp, Release (`/O2` MSVC, `-O3` + `--use_fast_math` nvcc) :

| Mode | Temps | Speedup |
|------|-------|---------|
| CPU single thread (V1) | 18.35s | 1x |
| CPU 20 threads | 820ms | 22x |
| GPU | 19ms | 966x |
| Hybride (70/30) | ~220ms | 83x |

## Tests

18 suites GTest : vec3, ray, interval, AABB, sphere, plane, triangle, cylinder, materials, camera, scene, BVH, image, random, cpu_renderer, gpu_renderer, hybrid_renderer.
