#include <gtest/gtest.h>
#include "scene/scene.h"
#include "scene/material.cuh"

TEST(Scene, BuildThreeSpheres) {
    Scene scene = Scene::build_three_spheres();
    // ground + 3 spheres + bulle = 5 objects
    EXPECT_EQ(scene.world.objects().size(), 5u);
    EXPECT_GT(scene.camera.get_image_width(), 0);
    EXPECT_GT(scene.camera.get_image_height(), 0);
}

TEST(Scene, BuildFinalScene) {
    Scene scene = Scene::build_final_scene();
    // Should have ground + many random spheres + 3 large spheres
    EXPECT_GT(scene.world.objects().size(), 10u);
}

TEST(Scene, BuildTriangleScene) {
    Scene scene = Scene::build_triangle_scene();
    // Sol + 6 triangles pyramide + 2 spheres = 9 objets
    EXPECT_EQ(scene.world.objects().size(), 9u);
    EXPECT_GT(scene.camera.get_image_width(), 0);
}

TEST(Scene, BuildCornellBox) {
    Scene scene = Scene::build_cornell_box();
    // 10 murs + 2 light + 2 spheres = 14 objets
    EXPECT_EQ(scene.world.objects().size(), 14u);
    EXPECT_EQ(scene.camera.get_image_width(), 600);
}

TEST(Scene, AddMaterial) {
    Scene scene;
    // The template add_material should return a valid pointer
    auto* mat = scene.add_material<Lambertian>(Color(1, 0, 0));
    EXPECT_NE(mat, nullptr);
}
