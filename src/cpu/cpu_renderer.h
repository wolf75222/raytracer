#ifndef RT_CPU_RENDERER_H
#define RT_CPU_RENDERER_H

#include "scene/scene.h"
#include "utils/image.h"
#include <memory>

class Hittable;

class CpuRenderer {
public:
    CpuRenderer(const Scene& scene, ImageBuffer& image);

    // Single-threaded render
    void render();

    // Multi-threaded render with tile decomposition
    void render_threaded(int num_threads = 0);  // 0 = auto-detect

    // Enable BVH acceleration (builds BVH from scene)
    void enable_bvh();

    // Render a single tile (used by hybrid renderer)
    void render_tile(int x0, int y0, int x1, int y1) const;

private:
    const Scene& scene_;
    ImageBuffer& image_;
    std::shared_ptr<Hittable> bvh_root_;
    bool use_bvh_ = false;

    const Hittable& get_world() const;
    Color ray_color(const Ray& r, const Hittable& world, int depth) const;
};

#endif // RT_CPU_RENDERER_H
