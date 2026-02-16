#include "core/scene.h"
#include "core/image.h"
#include "cpu/cpu_renderer.h"
#include <iostream>
#include <string>
#include <chrono>
#include <cstdlib>

int main(int argc, char* argv[]) {
    // Defaults
    std::string scene_name = "three_spheres";
    std::string output_file = "output/render.ppm";
    int width = 0;   // 0 = use scene default
    int spp = 0;     // 0 = use scene default
    int threads = 0; // 0 = auto-detect
    bool use_bvh = true;
    bool single_thread = false;

    // Simple argument parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scene" && i + 1 < argc) scene_name = argv[++i];
        else if (arg == "--output" && i + 1 < argc) output_file = argv[++i];
        else if (arg == "--width" && i + 1 < argc) width = std::atoi(argv[++i]);
        else if (arg == "--spp" && i + 1 < argc) spp = std::atoi(argv[++i]);
        else if (arg == "--threads" && i + 1 < argc) threads = std::atoi(argv[++i]);
        else if (arg == "--no-bvh") use_bvh = false;
        else if (arg == "--single-thread") single_thread = true;
        else if (arg == "--help") {
            std::cout << "Usage: raytracer [options]\n"
                      << "  --scene <name>       Scene: three_spheres, final (default: three_spheres)\n"
                      << "  --output <file>      Output PPM file (default: output/render.ppm)\n"
                      << "  --width <pixels>     Image width (default: scene default)\n"
                      << "  --spp <samples>      Samples per pixel (default: scene default)\n"
                      << "  --threads <n>        Number of threads (default: auto)\n"
                      << "  --no-bvh             Disable BVH acceleration\n"
                      << "  --single-thread      Use single-threaded renderer\n";
            return 0;
        }
    }

    // Build scene
    Scene scene;
    if (scene_name == "final")
        scene = Scene::build_final_scene();
    else
        scene = Scene::build_three_spheres();

    // Override settings if provided
    if (width > 0) {
        scene.camera.image_width = width;
        scene.camera.initialize();
    }
    if (spp > 0)
        scene.camera.samples_per_pixel = spp;

    // Create image buffer
    ImageBuffer image(scene.camera.get_image_width(), scene.camera.get_image_height());
    CpuRenderer renderer(scene, image);

    if (use_bvh) {
        std::cerr << "Building BVH...\n";
        renderer.enable_bvh();
    }

    std::cerr << "Rendering " << image.width() << "x" << image.height()
              << " (" << scene.camera.samples_per_pixel << " spp, depth "
              << scene.camera.max_depth << ")"
              << (use_bvh ? " [BVH]" : "") << "\n";

    // Render and time
    auto start = std::chrono::high_resolution_clock::now();
    if (single_thread)
        renderer.render();
    else
        renderer.render_threaded(threads);
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cerr << "Render time: " << elapsed << "s\n";

    // Save
    image.write_ppm(output_file);
    std::cerr << "Saved to: " << output_file << "\n";

    return 0;
}
