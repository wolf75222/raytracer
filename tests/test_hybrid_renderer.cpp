#include <gtest/gtest.h>
#include "scene/scene.h"
#include "utils/image.h"
#include "cpu/cpu_renderer.h"
#include <cmath>

#ifdef RT_CUDA_ENABLED
#include "gpu/gpu_renderer.h"
#include "hybrid/hybrid_renderer.h"
#endif

static double compute_rmse(const ImageBuffer& a, const ImageBuffer& b) {
    int w = a.width(), h = a.height();
    if (w != b.width() || h != b.height()) return 1e10;
    double sum = 0.0;
    int count = 0;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            Color ca = a.get_pixel(x, y);
            Color cb = b.get_pixel(x, y);
            double dr = ca.x() - cb.x(), dg = ca.y() - cb.y(), db = ca.z() - cb.z();
            sum += dr*dr + dg*dg + db*db;
            count += 3;
        }
    return std::sqrt(sum / count);
}

#ifdef RT_CUDA_ENABLED

TEST(HybridRenderer, ProducesNonBlackImage) {
    Scene scene = Scene::build_three_spheres();
    scene.camera.image_width = 80;
    scene.camera.samples_per_pixel = 4;
    scene.camera.max_depth = 10;
    scene.camera.initialize();

    ImageBuffer image(scene.camera.get_image_width(), scene.camera.get_image_height());
    float cpu_ms = 0, gpu_ms = 0;
    hybrid_render_scene(scene, image, 0.5f, cpu_ms, gpu_ms);

    int nonblack = 0;
    for (int y = 0; y < image.height(); y++)
        for (int x = 0; x < image.width(); x++) {
            Color c = image.get_pixel(x, y);
            if (c.x() + c.y() + c.z() > 0.01) nonblack++;
        }
    EXPECT_GT(nonblack, image.width() * image.height() / 2);
}

TEST(HybridRenderer, GpuFrac0IsCpuOnly) {
    Scene scene = Scene::build_three_spheres();
    scene.camera.image_width = 40;
    scene.camera.samples_per_pixel = 4;
    scene.camera.max_depth = 5;
    scene.camera.defocus_angle = 0.0;
    scene.camera.initialize();

    int w = scene.camera.get_image_width();
    int h = scene.camera.get_image_height();

    // Hybrid avec gpu_frac=0 -> CPU seul
    ImageBuffer hybrid_img(w, h);
    float cpu_ms = 0, gpu_ms = 0;
    hybrid_render_scene(scene, hybrid_img, 0.0f, cpu_ms, gpu_ms);

    // Doit avoir rendu quelque chose
    int nonblack = 0;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            Color c = hybrid_img.get_pixel(x, y);
            if (c.x() + c.y() + c.z() > 0.01) nonblack++;
        }
    EXPECT_GT(nonblack, w * h / 2);
}

TEST(HybridRenderer, GpuFrac1IsGpuOnly) {
    Scene scene = Scene::build_three_spheres();
    scene.camera.image_width = 40;
    scene.camera.samples_per_pixel = 4;
    scene.camera.max_depth = 5;
    scene.camera.initialize();

    int w = scene.camera.get_image_width();
    int h = scene.camera.get_image_height();

    // gpu_frac=1.0 -> GPU rend tout
    ImageBuffer hybrid_img(w, h);
    float cpu_ms = 0, gpu_ms = 0;
    hybrid_render_scene(scene, hybrid_img, 1.0f, cpu_ms, gpu_ms);

    // GPU doit avoir fonctionne
    EXPECT_GT(gpu_ms, 0.0f);
}

TEST(HybridRenderer, SimilarToCpu) {
    Scene scene = Scene::build_three_spheres();
    scene.camera.image_width = 40;
    scene.camera.samples_per_pixel = 50;
    scene.camera.max_depth = 10;
    scene.camera.defocus_angle = 0.0;
    scene.camera.initialize();

    int w = scene.camera.get_image_width();
    int h = scene.camera.get_image_height();

    // CPU reference
    ImageBuffer cpu_img(w, h);
    CpuRenderer cpu_renderer(scene, cpu_img);
    cpu_renderer.enable_bvh();
    cpu_renderer.render_threaded(0);

    // Hybrid 50/50
    ImageBuffer hybrid_img(w, h);
    float cpu_ms = 0, gpu_ms = 0;
    hybrid_render_scene(scene, hybrid_img, 0.5f, cpu_ms, gpu_ms);

    // RMSE devrait etre raisonnable (float vs double + RNG differents)
    double rmse = compute_rmse(cpu_img, hybrid_img);
    EXPECT_LT(rmse, 0.2);
}

#else

TEST(HybridRenderer, Disabled) {
    GTEST_SKIP() << "CUDA not enabled";
}

#endif
