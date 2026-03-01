#include <gtest/gtest.h>
#include "core/scene.h"
#include "core/lambertian.h"

TEST(Scene, BuildThreeSpheres) {
    Scene scene = Scene::build_three_spheres();
    // Should have ground plane + 3 spheres = 4 objects
    EXPECT_EQ(scene.world.objects().size(), 4u);
    EXPECT_GT(scene.camera.get_image_width(), 0);
    EXPECT_GT(scene.camera.get_image_height(), 0);
}

TEST(Scene, BuildFinalScene) {
    Scene scene = Scene::build_final_scene();
    // Should have ground + many random spheres + 3 large spheres
    EXPECT_GT(scene.world.objects().size(), 10u);
}

TEST(Scene, AddMaterial) {
    Scene scene;
    // The template add_material should return a valid pointer
    auto* mat = scene.add_material<Lambertian>(Color(1, 0, 0));
    EXPECT_NE(mat, nullptr);
}
