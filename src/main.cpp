#include "scene/scene.h"
#include "utils/image.h"
#include "utils/denoise.h"
#include "cpu/cpu_renderer.h"
#include <iostream>
#include <string>
#include <chrono>
#include <cstdlib>

#ifdef RT_CUDA_ENABLED
#include "gpu/gpu_renderer.h"
#include "gpu/gpu_realtime.h"
#include "hybrid/hybrid_renderer.h"
#endif

int main(int argc, char* argv[]) {
    std::string scene_name = "three_spheres";
    std::string output_file = "output/render.ppm";
    int width = 0;
    int spp = 0;
    int threads = 0;
    bool use_bvh = true;
    bool single_thread = false;
    bool use_gpu = false;
    bool use_hybrid = false;
    bool use_realtime = false;
    bool use_denoise = false;
    bool use_denoise_gpu = false;
    float gpu_frac = 0.7f;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scene" && i + 1 < argc) scene_name = argv[++i];
        else if (arg == "--output" && i + 1 < argc) output_file = argv[++i];
        else if (arg == "--width" && i + 1 < argc) width = std::atoi(argv[++i]);
        else if (arg == "--spp" && i + 1 < argc) spp = std::atoi(argv[++i]);
        else if (arg == "--threads" && i + 1 < argc) threads = std::atoi(argv[++i]);
        else if (arg == "--no-bvh") use_bvh = false;
        else if (arg == "--single-thread") single_thread = true;
        else if (arg == "--gpu") use_gpu = true;
        else if (arg == "--hybrid") use_hybrid = true;
        else if (arg == "--realtime") use_realtime = true;
        else if (arg == "--denoise") use_denoise = true;
        else if (arg == "--denoise-gpu") use_denoise_gpu = true;
        else if (arg == "--gpu-frac" && i + 1 < argc) gpu_frac = (float)std::atof(argv[++i]);
        else if (arg == "--help") {
            std::cout << "Usage: raytracer [options]\n"
                      << "  --scene <name>       Scene: three_spheres, final\n"
                      << "  --output <file>      Output PPM file\n"
                      << "  --width <pixels>     Image width\n"
                      << "  --spp <samples>      Samples per pixel\n"
                      << "  --threads <n>        Number of CPU threads (0=auto)\n"
                      << "  --no-bvh             Disable BVH\n"
                      << "  --single-thread      Single-threaded CPU\n"
                      << "  --gpu                Use GPU (CUDA) renderer\n"
                      << "  --hybrid             Use hybrid CPU+GPU renderer\n"
                      << "  --gpu-frac <0-1>     GPU fraction in hybrid mode (default: 0.7)\n";
            return 0;
        }
    }

    Scene scene;
    if (scene_name == "final")
        scene = Scene::build_final_scene();
    else if (scene_name == "triangle")
        scene = Scene::build_triangle_scene();
    else if (scene_name == "cornell")
        scene = Scene::build_cornell_box();
    else if (scene_name == "showcase")
        scene = Scene::build_showcase();
    else if (scene_name == "hall")
        scene = Scene::build_hall();
    else if (scene_name == "galaxy")
        scene = Scene::build_galaxy();
    else if (scene_name == "minimal")
        scene = Scene::build_minimal();
    else
        scene = Scene::build_three_spheres();

    if (width > 0) {
        scene.camera.image_width = width;
        scene.camera.initialize();
    }
    if (spp > 0)
        scene.camera.samples_per_pixel = spp;

    int img_w = scene.camera.get_image_width();
    int img_h = scene.camera.get_image_height();
    ImageBuffer image(img_w, img_h);

    std::cerr << "Rendering " << img_w << "x" << img_h
              << " (" << scene.camera.samples_per_pixel << " spp, depth "
              << scene.camera.max_depth << ")";

#ifdef RT_CUDA_ENABLED
    if (use_realtime) {
        std::cerr << " [REALTIME GPU]\n";
        int rt_w = (width > 0) ? width : 640;
        int rt_h = (int)(rt_w / scene.camera.aspect_ratio);
        scene.camera.image_width = rt_w;
        scene.camera.initialize();
        gpu_realtime_render(scene, rt_w, rt_h);
        return 0;
    }

    if (use_hybrid) {
        std::cerr << " [HYBRID gpu=" << (int)(gpu_frac*100) << "%]\n";
        float cpu_ms = 0, gpu_ms = 0;
        auto start = std::chrono::high_resolution_clock::now();
        hybrid_render_scene(scene, image, gpu_frac, cpu_ms, gpu_ms);
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        std::cerr << "Total: " << elapsed << "s\n";
        image.write_ppm(output_file);
        std::cerr << "Saved to: " << output_file << "\n";
        return 0;
    }

    if (use_gpu) {
        std::cerr << (use_denoise_gpu ? " [GPU+DENOISE]\n" : " [GPU]\n");
        float kernel_ms = 0;
        auto start = std::chrono::high_resolution_clock::now();
        if (use_denoise_gpu)
            gpu_render_scene_denoised(scene, image, kernel_ms);
        else
            gpu_render_scene(scene, image, kernel_ms);
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        std::cerr << "GPU kernel: " << kernel_ms << " ms\n";
        std::cerr << "Total (incl. transfer): " << elapsed << "s\n";
        image.write_ppm(output_file);
        std::cerr << "Saved to: " << output_file << "\n";
        return 0;
    }
#else
    if (use_gpu || use_hybrid) {
        std::cerr << "\nError: GPU not compiled. Rebuild with -DENABLE_CUDA=ON\n";
        return 1;
    }
#endif

    std::cerr << (use_bvh ? " [BVH]" : "") << "\n";
    CpuRenderer renderer(scene, image);
    if (use_bvh) {
        std::cerr << "Building BVH...\n";
        renderer.enable_bvh();
    }

    auto start = std::chrono::high_resolution_clock::now();
    if (single_thread)
        renderer.render();
    else
        renderer.render_threaded(threads);
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cerr << "Render time: " << elapsed << "s\n";
    if (use_denoise) {
        std::cerr << "Denoising...\n";
        denoise_bilateral(image, 3, 0.1);
    }
    image.write_ppm(output_file);
    std::cerr << "Saved to: " << output_file << "\n";

    return 0;
}
