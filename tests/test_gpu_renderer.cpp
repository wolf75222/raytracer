#include <gtest/gtest.h>
#include "scene/scene.h"
#include "utils/image.h"
#include "cpu/cpu_renderer.h"

#ifdef RT_CUDA_ENABLED
#include "gpu/gpu_renderer.h"
#endif

// Calcul RMSE entre deux images
static double compute_rmse(const ImageBuffer& a, const ImageBuffer& b) {
    int w = a.width(), h = a.height();
    if (w != b.width() || h != b.height()) return 1e10;

    double sum = 0.0;
    int count = 0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Color ca = a.get_pixel(x, y);
            Color cb = b.get_pixel(x, y);
            double dr = ca.x() - cb.x();
            double dg = ca.y() - cb.y();
            double db = ca.z() - cb.z();
            sum += dr*dr + dg*dg + db*db;
            count += 3;
        }
    }
    return std::sqrt(sum / count);
}

#ifdef RT_CUDA_ENABLED

TEST(GpuRenderer, ProducesNonBlackImage) {
    Scene scene = Scene::build_three_spheres();
    scene.camera.image_width = 80;
    scene.camera.samples_per_pixel = 4;
    scene.camera.max_depth = 10;
    scene.camera.initialize();

    ImageBuffer image(scene.camera.get_image_width(), scene.camera.get_image_height());
    float kernel_ms = 0;
    gpu_render_scene(scene, image, kernel_ms);

    // Au moins quelques pixels non noirs
    int nonblack = 0;
    for (int y = 0; y < image.height(); y++)
        for (int x = 0; x < image.width(); x++) {
            Color c = image.get_pixel(x, y);
            if (c.x() + c.y() + c.z() > 0.01) nonblack++;
        }
    EXPECT_GT(nonblack, image.width() * image.height() / 2);
    EXPECT_GT(kernel_ms, 0.0f);
}

TEST(GpuRenderer, SimilarToCpuOutput) {
    Scene scene = Scene::build_three_spheres();
    scene.camera.image_width = 40;
    scene.camera.samples_per_pixel = 50;  // assez pour converger
    scene.camera.max_depth = 10;
    scene.camera.defocus_angle = 0.0;  // pas de DOF pour comparaison deterministe
    scene.camera.initialize();

    int w = scene.camera.get_image_width();
    int h = scene.camera.get_image_height();

    // Rendu CPU
    ImageBuffer cpu_image(w, h);
    CpuRenderer cpu_renderer(scene, cpu_image);
    cpu_renderer.enable_bvh();
    cpu_renderer.render_threaded(0);

    // Rendu GPU
    ImageBuffer gpu_image(w, h);
    float kernel_ms = 0;
    gpu_render_scene(scene, gpu_image, kernel_ms);

    // RMSE entre les deux (tolere des differences dues au float vs double et RNG differents)
    double rmse = compute_rmse(cpu_image, gpu_image);
    // Les images doivent etre similaires (RMSE < 0.15 car RNG et precision differents)
    EXPECT_LT(rmse, 0.15);
}

TEST(GpuRenderer, FinalSceneRenders) {
    Scene scene = Scene::build_final_scene();
    scene.camera.image_width = 40;
    scene.camera.samples_per_pixel = 2;
    scene.camera.max_depth = 5;
    scene.camera.initialize();

    ImageBuffer image(scene.camera.get_image_width(), scene.camera.get_image_height());
    float kernel_ms = 0;
    gpu_render_scene(scene, image, kernel_ms);

    // Doit s'executer sans crash sur une scene avec ~480 objets
    EXPECT_GT(kernel_ms, 0.0f);
}

#else

TEST(GpuRenderer, Disabled) {
    GTEST_SKIP() << "CUDA not enabled";
}

#endif
