#include "cpu/cpu_renderer.h"
#include "core/random_utils.h"
#include "core/interval.h"
#include "core/material.h"
#include "core/lambertian.h"
#include "core/metal.h"
#include "core/dielectric.h"
#include "core/bvh.h"
#include <iostream>
#include <limits>
#include <thread>
#include <atomic>
#include <vector>
#include <cmath>

CpuRenderer::CpuRenderer(const Scene& scene, ImageBuffer& image)
    : scene_(scene), image_(image) {}

void CpuRenderer::enable_bvh() {
    bvh_root_ = std::make_shared<BVHAccel>(scene_.world);
    use_bvh_ = true;
}

const Hittable& CpuRenderer::get_world() const {
    if (use_bvh_ && bvh_root_)
        return *bvh_root_;
    return scene_.world;
}

// Inline scatter to avoid virtual dispatch in the hot path
static inline bool scatter_inline(const Material* mat, const Ray& r_in,
                                   const HitRecord& rec,
                                   Color& attenuation, Ray& scattered) {
    switch (mat->type()) {
    case Material::Type::Lambertian: {
        const auto* lam = static_cast<const Lambertian*>(mat);
        Vec3 scatter_dir = rec.normal + random_unit_vector();
        if (scatter_dir.near_zero()) scatter_dir = rec.normal;
        scattered = Ray(rec.p, scatter_dir);
        attenuation = lam->albedo();
        return true;
    }
    case Material::Type::Metal: {
        const auto* met = static_cast<const Metal*>(mat);
        Vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        reflected = reflected + met->fuzz() * random_in_unit_sphere();
        scattered = Ray(rec.p, reflected);
        attenuation = met->albedo();
        return (dot(scattered.direction(), rec.normal) > 0.0);
    }
    case Material::Type::Dielectric: {
        const auto* die = static_cast<const Dielectric*>(mat);
        attenuation = Color(1.0, 1.0, 1.0);
        double ri_val = die->refraction_index();
        double ri = rec.front_face ? (1.0 / ri_val) : ri_val;
        Vec3 unit_dir = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_dir, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);
        bool cannot_refract = ri * sin_theta > 1.0;
        Vec3 direction;
        if (cannot_refract || Dielectric::reflectance(cos_theta, ri) > random_double())
            direction = reflect(unit_dir, rec.normal);
        else
            direction = refract(unit_dir, rec.normal, ri);
        scattered = Ray(rec.p, direction);
        return true;
    }
    default:
        // Fallback to virtual dispatch for unknown types
        return mat->scatter(r_in, rec, attenuation, scattered);
    }
}

void CpuRenderer::render() {
    const int width = image_.width();
    const int height = image_.height();
    const int spp = scene_.camera.samples_per_pixel;
    const int max_depth = scene_.camera.max_depth;
    const double inv_spp = 1.0 / spp;
    const Hittable& world = get_world();
    const Camera& cam = scene_.camera;

    for (int j = 0; j < height; ++j) {
        std::cerr << "\rScanlines remaining: " << (height - j) << ' ' << std::flush;
        for (int i = 0; i < width; ++i) {
            Color pixel_color(0, 0, 0);
            for (int s = 0; s < spp; ++s) {
                Ray r = cam.get_ray(i, j);
                pixel_color += ray_color(r, world, max_depth);
            }
            image_.set_pixel(i, j, pixel_color * inv_spp);
        }
    }
    std::cerr << "\rDone.                          \n";
}

void CpuRenderer::render_tile(int x0, int y0, int x1, int y1) const {
    const int spp = scene_.camera.samples_per_pixel;
    const int max_depth = scene_.camera.max_depth;
    const double inv_spp = 1.0 / spp;
    const Hittable& world = get_world();
    const Camera& cam = scene_.camera;

    // Process samples in pairs for better ILP
    const int spp_pairs = spp / 2;
    const int spp_remainder = spp % 2;

    for (int j = y0; j < y1; ++j) {
        for (int i = x0; i < x1; ++i) {
            Color acc0(0, 0, 0);
            Color acc1(0, 0, 0);

            for (int s = 0; s < spp_pairs; ++s) {
                Ray r0 = cam.get_ray(i, j);
                acc0 += ray_color(r0, world, max_depth);
                Ray r1 = cam.get_ray(i, j);
                acc1 += ray_color(r1, world, max_depth);
            }
            if (spp_remainder) {
                Ray r = cam.get_ray(i, j);
                acc0 += ray_color(r, world, max_depth);
            }

            image_.set_pixel(i, j, (acc0 + acc1) * inv_spp);
        }
    }
}

void CpuRenderer::render_threaded(int num_threads) {
    if (num_threads <= 0)
        num_threads = static_cast<int>(std::thread::hardware_concurrency());
    if (num_threads <= 0)
        num_threads = 4;

    const int width = image_.width();
    const int height = image_.height();
    constexpr int TILE_SIZE = 16;

    const int tiles_x = (width + TILE_SIZE - 1) / TILE_SIZE;
    const int tiles_y = (height + TILE_SIZE - 1) / TILE_SIZE;
    const int total_tiles = tiles_x * tiles_y;

    std::atomic<int> next_tile(0);
    std::atomic<int> completed_tiles(0);

    auto worker = [&]() {
        while (true) {
            int tile_idx = next_tile.fetch_add(1);
            if (tile_idx >= total_tiles)
                break;

            int tx = tile_idx % tiles_x;
            int ty = tile_idx / tiles_x;
            int x0 = tx * TILE_SIZE;
            int y0 = ty * TILE_SIZE;
            int x1 = std::min(x0 + TILE_SIZE, width);
            int y1 = std::min(y0 + TILE_SIZE, height);

            render_tile(x0, y0, x1, y1);

            int done = completed_tiles.fetch_add(1) + 1;
            if (done % 10 == 0 || done == total_tiles) {
                std::cerr << "\rTiles: " << done << "/" << total_tiles
                          << " (" << (100 * done / total_tiles) << "%)" << std::flush;
            }
        }
    };

    std::cerr << "Rendering with " << num_threads << " threads, "
              << total_tiles << " tiles (" << TILE_SIZE << "x" << TILE_SIZE << ")...\n";

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i)
        threads.emplace_back(worker);

    for (auto& t : threads)
        t.join();

    std::cerr << "\rDone.                                          \n";
}

// CS:APP 5.5 + 5.6: Iterative ray_color with devirtualized scatter
// - No recursion (eliminates stack frame per bounce)
// - Inline scatter avoids vtable lookup per bounce
// - Local variables eliminate repeated memory dereferences
Color CpuRenderer::ray_color(const Ray& r, const Hittable& world, int depth) const {
    Color throughput(1, 1, 1);
    Ray current_ray = r;
    constexpr double T_MIN = 0.001;
    constexpr double T_MAX = 1e30; // CS:APP 5.6: avoid std::numeric_limits call

    for (int d = 0; d < depth; ++d) {
        HitRecord rec;
        if (!world.hit(current_ray, Interval(T_MIN, T_MAX), rec)) {
            // Sky: CS:APP 5.6 - compute with local values
            const Vec3& dir = current_ray.direction();
            // Inline unit_vector Y component to avoid full normalization
            // We only need the Y component for the sky gradient
            double len_sq = dir.length_squared();
            double inv_len = 1.0 / std::sqrt(len_sq);
            double unit_y = dir.y() * inv_len;
            double a = 0.5 * (unit_y + 1.0);
            // Sky color: lerp white->blue
            Color sky(1.0 - 0.5 * a, 1.0 - 0.3 * a, 1.0);
            return throughput * sky;
        }

        // CS:APP 5.5: Devirtualized scatter - switch on type tag, no vtable
        const Material* mat = rec.material;
        if (!mat) break;

        Ray scattered;
        Color attenuation;
        if (!scatter_inline(mat, current_ray, rec, attenuation, scattered))
            break; // absorbed

        // CS:APP 5.6: Multiply throughput components directly
        throughput = throughput * attenuation;
        current_ray = scattered;
    }

    return Color(0, 0, 0); // max depth reached
}
