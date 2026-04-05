// Hybrid CPU+GPU renderer avec work stealing par tuiles
// Le GPU rend la partie haute, le CPU la partie basse, en parallele

#include "hybrid/hybrid_renderer.h"
#include "gpu/gpu_renderer.h"
#include "gpu/gpu_types.cuh"
#include "scene/primitives.cuh"
#include "scene/material.cuh"
#include "scene/bvh_builder.h"
#include "cpu/cpu_renderer.h"
#include "math/random.cuh"
#include "math/interval.cuh"
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <vector>
#include <cmath>

static float s_adaptive_fraction = -1.0f;

void hybrid_render_scene(const Scene& scene, ImageBuffer& image,
                         float gpu_fraction,
                         float& cpu_time_ms, float& gpu_time_ms) {
    int w = image.width();
    int h = image.height();
    constexpr int TILE_SIZE = 32;

    // Auto-balancing apres la premiere frame (reset si fraction change)
    static float s_last_requested = -1.0f;
    if (s_last_requested >= 0.0f && std::fabs(gpu_fraction - s_last_requested) > 0.1f)
        s_adaptive_fraction = -1.0f;
    s_last_requested = gpu_fraction;
    if (s_adaptive_fraction > 0.0f)
        gpu_fraction = s_adaptive_fraction;

    int tiles_x = (w + TILE_SIZE - 1) / TILE_SIZE;
    int tiles_y = (h + TILE_SIZE - 1) / TILE_SIZE;
    int total_tiles = tiles_x * tiles_y;

    int gpu_tiles = (int)(total_tiles * gpu_fraction);
    if (gpu_tiles < 0) gpu_tiles = 0;
    if (gpu_tiles > total_tiles) gpu_tiles = total_tiles;
    int cpu_start_tile = gpu_tiles;

    int gpu_rows = 0;
    if (gpu_tiles > 0) {
        int gpu_tile_rows = (gpu_tiles + tiles_x - 1) / tiles_x;
        gpu_rows = gpu_tile_rows * TILE_SIZE;
        if (gpu_rows > h) gpu_rows = h;
        gpu_tiles = gpu_tile_rows * tiles_x;
        cpu_start_tile = gpu_tiles;
    }

    fprintf(stderr, "Hybrid: GPU %d tuiles (lignes 0-%d), CPU %d tuiles (lignes %d-%d), total %d\n",
            gpu_tiles, gpu_rows, total_tiles - cpu_start_tile, gpu_rows, h, total_tiles);

    // --- GPU thread : rend dans un buffer separe ---
    std::thread gpu_thread;
    float gpu_kernel_ms = 0;
    ImageBuffer gpu_image(w, h); // buffer separe, pas de race

    if (gpu_rows > 0) {
        gpu_thread = std::thread([&]() {
            auto t0 = std::chrono::high_resolution_clock::now();
            float km = 0;
            gpu_render_scene(scene, gpu_image, km);
            gpu_kernel_ms = km;
            auto t1 = std::chrono::high_resolution_clock::now();
            gpu_time_ms = (float)std::chrono::duration<double, std::milli>(t1 - t0).count();
        });
    }

    // --- CPU : rend uniquement ses tuiles ---
    if (cpu_start_tile < total_tiles) {
        auto t0 = std::chrono::high_resolution_clock::now();

        CpuRenderer cpu_renderer(scene, image);
        cpu_renderer.enable_bvh();

        int num_threads = (int)std::thread::hardware_concurrency();
        if (num_threads <= 0) num_threads = 4;

        int cpu_total = total_tiles - cpu_start_tile;
        std::atomic<int> next_tile(0);

        auto worker = [&]() {
            while (true) {
                int local_idx = next_tile.fetch_add(1);
                if (local_idx >= cpu_total) break;

                int tile_idx = cpu_start_tile + local_idx;
                int tx = tile_idx % tiles_x;
                int ty = tile_idx / tiles_x;
                int x0 = tx * TILE_SIZE;
                int y0 = ty * TILE_SIZE;
                int x1 = std::min(x0 + TILE_SIZE, w);
                int y1 = std::min(y0 + TILE_SIZE, h);

                cpu_renderer.render_tile(x0, y0, x1, y1);
            }
        };

        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; i++)
            threads.emplace_back(worker);
        for (auto& t : threads)
            t.join();

        auto t1 = std::chrono::high_resolution_clock::now();
        cpu_time_ms = (float)std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    if (gpu_thread.joinable())
        gpu_thread.join();

    // Copie GPU -> image (seulement les lignes GPU, apres join = pas de race)
    if (gpu_rows > 0) {
        const Color* gpu_data = gpu_image.data();
        Color* dst = image.data();
        for (int y = 0; y < gpu_rows; y++)
            for (int x = 0; x < w; x++)
                dst[y * w + x] = gpu_data[y * w + x];
    }

    // Auto-balancing pour les prochains appels
    if (gpu_rows > 0 && gpu_rows < h && cpu_time_ms > 0 && gpu_time_ms > 0) {
        float gpu_pix_per_ms = (float)(w * gpu_rows) / gpu_time_ms;
        float cpu_pix_per_ms = (float)(w * (h - gpu_rows)) / cpu_time_ms;
        float optimal = gpu_pix_per_ms / (gpu_pix_per_ms + cpu_pix_per_ms);
        if (s_adaptive_fraction < 0.0f)
            s_adaptive_fraction = optimal;
        else
            s_adaptive_fraction = 0.7f * s_adaptive_fraction + 0.3f * optimal;
    }

    fprintf(stderr, "Hybrid: CPU %.1fms, GPU kernel %.1fms (total GPU %.1fms)\n",
            cpu_time_ms, gpu_kernel_ms, gpu_time_ms);
}
