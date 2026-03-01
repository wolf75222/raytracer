#include "core/scene.h"
#include "core/sphere.h"
#include "core/plane.h"
#include "core/lambertian.h"
#include "core/metal.h"
#include "core/dielectric.h"
#include "core/random_utils.h"

Scene Scene::build_three_spheres() {
    Scene scene;

    // Materials
    auto* mat_ground = scene.add_material<Lambertian>(Color(0.5, 0.5, 0.5));
    auto* mat_center = scene.add_material<Lambertian>(Color(0.1, 0.2, 0.5));
    auto* mat_left   = scene.add_material<Dielectric>(1.5);
    auto* mat_right  = scene.add_material<Metal>(Color(0.8, 0.6, 0.2), 0.0);

    // Objects
    scene.world.add(std::make_shared<Plane>(Point3(0, 0, 0), Vec3(0, 1, 0), mat_ground));
    scene.world.add(std::make_shared<Sphere>(Point3(0.0, 0.5, -1.2), 0.5, mat_center));
    scene.world.add(std::make_shared<Sphere>(Point3(-1.0, 0.5, -1.2), 0.5, mat_left));
    scene.world.add(std::make_shared<Sphere>(Point3(1.0, 0.5, -1.2), 0.5, mat_right));

    // Camera
    scene.camera.aspect_ratio = 16.0 / 9.0;
    scene.camera.image_width = 800;
    scene.camera.samples_per_pixel = 100;
    scene.camera.max_depth = 50;
    scene.camera.vfov = 20.0;
    scene.camera.lookfrom = Point3(13, 2, 3);
    scene.camera.lookat = Point3(0, 0.5, -1);
    scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.6;
    scene.camera.focus_dist = 10.0;
    scene.camera.initialize();

    return scene;
}

Scene Scene::build_final_scene() {
    Scene scene;

    // Ground
    auto* ground_mat = scene.add_material<Lambertian>(Color(0.5, 0.5, 0.5));
    scene.world.add(std::make_shared<Plane>(Point3(0, 0, 0), Vec3(0, 1, 0), ground_mat));

    // Random small spheres
    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            double choose_mat = random_double();
            Point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - Point3(4, 0.2, 0)).length() > 0.9) {
                if (choose_mat < 0.8) {
                    // Diffuse
                    Color albedo = random_vec3() * random_vec3();
                    auto* mat = scene.add_material<Lambertian>(albedo);
                    scene.world.add(std::make_shared<Sphere>(center, 0.2, mat));
                } else if (choose_mat < 0.95) {
                    // Metal
                    Color albedo = random_vec3(0.5, 1.0);
                    double fuzz = random_double(0.0, 0.5);
                    auto* mat = scene.add_material<Metal>(albedo, fuzz);
                    scene.world.add(std::make_shared<Sphere>(center, 0.2, mat));
                } else {
                    // Glass
                    auto* mat = scene.add_material<Dielectric>(1.5);
                    scene.world.add(std::make_shared<Sphere>(center, 0.2, mat));
                }
            }
        }
    }

    // Three large spheres
    auto* mat1 = scene.add_material<Dielectric>(1.5);
    scene.world.add(std::make_shared<Sphere>(Point3(0, 1, 0), 1.0, mat1));

    auto* mat2 = scene.add_material<Lambertian>(Color(0.4, 0.2, 0.1));
    scene.world.add(std::make_shared<Sphere>(Point3(-4, 1, 0), 1.0, mat2));

    auto* mat3 = scene.add_material<Metal>(Color(0.7, 0.6, 0.5), 0.0);
    scene.world.add(std::make_shared<Sphere>(Point3(4, 1, 0), 1.0, mat3));

    // Camera
    scene.camera.aspect_ratio = 16.0 / 9.0;
    scene.camera.image_width = 1200;
    scene.camera.samples_per_pixel = 500;
    scene.camera.max_depth = 50;
    scene.camera.vfov = 20.0;
    scene.camera.lookfrom = Point3(13, 2, 3);
    scene.camera.lookat = Point3(0, 0, 0);
    scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.6;
    scene.camera.focus_dist = 10.0;
    scene.camera.initialize();

    return scene;
}
