#include "scene/scene.h"
#include "math/random.cuh"
#include <cmath>

// ============================================================
// Scene "three_spheres" : 3 materiaux sur sol gris
// Plan + 3 spheres + bulle interne = 5 objets
// ============================================================
Scene Scene::build_three_spheres() {
    Scene scene;

    auto* mat_ground = scene.add_material<Lambertian>(Color(0.45, 0.45, 0.42));
    auto* mat_terre = scene.add_material<Lambertian>(Color(0.6, 0.25, 0.1));
    auto* mat_chrome = scene.add_material<Metal>(Color(0.9, 0.9, 0.92), 0.0);
    auto* mat_eau = scene.add_material<Dielectric>(1.33);

    scene.world.add(std::make_shared<Plane>(Point3(0, 0, 0), Vec3(0, 1, 0), mat_ground));

    // Disposition en triangle : avant-gauche, arriere-centre, avant-droite
    scene.world.add(std::make_shared<Sphere>(Point3(-1.5, 0.6, 1.0), 0.6, mat_terre));
    scene.world.add(std::make_shared<Sphere>(Point3(0, 1.1, -1.2), 1.1, mat_eau));
    scene.world.add(std::make_shared<Sphere>(Point3(1.8, 0.5, 0.5), 0.5, mat_chrome));
    // Bulle dans la sphere d'eau
    scene.world.add(std::make_shared<Sphere>(Point3(0, 1.1, -1.2), -1.0, mat_eau));

    scene.camera.aspect_ratio = 16.0 / 9.0;
    scene.camera.image_width = 800;
    scene.camera.samples_per_pixel = 100;
    scene.camera.max_depth = 50;
    scene.camera.vfov = 40.0;
    scene.camera.lookfrom = Point3(4.0, 2.5, 4.5);
    scene.camera.lookat = Point3(0, 0.4, -0.2);
    scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.3;
    scene.camera.focus_dist = 6.0;
    scene.camera.initialize();

    return scene;
}

// ============================================================
// Scene "final" : grille concentrique de spheres (stress test)
// Anneaux concentriques autour d'une sphere centrale, hauteurs variees
// ============================================================
Scene Scene::build_final_scene() {
    Scene scene;

    auto* ground = scene.add_material<Lambertian>(Color(0.5, 0.5, 0.48));
    scene.world.add(std::make_shared<Plane>(Point3(0, 0, 0), Vec3(0, 1, 0), ground));

    // Sphere centrale : verre
    auto* m_center = scene.add_material<Dielectric>(1.5);
    scene.world.add(std::make_shared<Sphere>(Point3(0, 1.5, 0), 1.5, m_center));

    // Anneaux concentriques
    int n_rings = 6;
    for (int ring = 1; ring <= n_rings; ++ring) {
        double radius = ring * 2.0;
        int count = ring * 8;
        for (int i = 0; i < count; ++i) {
            double angle = 2.0 * 3.14159265 * i / count + ring * 0.3;
            double x = radius * std::cos(angle);
            double z = radius * std::sin(angle);
            double r = 0.15 + random_double() * 0.15;
            double y = r + random_double() * 0.1 * ring;

            double choose = random_double();
            if (choose < 0.5) {
                Color c(random_double(0.15, 0.85), random_double(0.15, 0.85), random_double(0.15, 0.85));
                auto* mat = scene.add_material<Lambertian>(c);
                scene.world.add(std::make_shared<Sphere>(Point3(x, y, z), r, mat));
            } else if (choose < 0.82) {
                Color c(random_double(0.5, 1.0), random_double(0.5, 1.0), random_double(0.5, 1.0));
                auto* mat = scene.add_material<Metal>(c, random_double(0.0, 0.35));
                scene.world.add(std::make_shared<Sphere>(Point3(x, y, z), r, mat));
            } else {
                auto* mat = scene.add_material<Dielectric>(1.4 + random_double() * 0.3);
                scene.world.add(std::make_shared<Sphere>(Point3(x, y, z), r, mat));
            }
        }
    }

    // 2 grosses spheres decoratives
    auto* m_gold = scene.add_material<Metal>(Color(0.85, 0.65, 0.2), 0.1);
    scene.world.add(std::make_shared<Sphere>(Point3(-4.0, 1.0, 3.0), 1.0, m_gold));

    auto* m_rouge = scene.add_material<Lambertian>(Color(0.7, 0.1, 0.1));
    scene.world.add(std::make_shared<Sphere>(Point3(3.5, 0.8, -4.0), 0.8, m_rouge));

    scene.camera.aspect_ratio = 16.0 / 9.0;
    scene.camera.image_width = 800;
    scene.camera.samples_per_pixel = 50;
    scene.camera.max_depth = 50;
    scene.camera.vfov = 30.0;
    scene.camera.lookfrom = Point3(12, 5, 10);
    scene.camera.lookat = Point3(0, 0.8, 0);
    scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.3;
    scene.camera.focus_dist = 15.0;
    scene.camera.initialize();

    return scene;
}

// ============================================================
// Scene "triangle" : pyramide + formes geometriques
// ============================================================
Scene Scene::build_triangle_scene() {
    Scene scene;

    auto* mat_ground = scene.add_material<Checker>(Color(0.8, 0.75, 0.7), Color(0.35, 0.3, 0.3), 1.0);
    auto* mat_red    = scene.add_material<Lambertian>(Color(0.75, 0.12, 0.12));
    auto* mat_teal   = scene.add_material<Lambertian>(Color(0.1, 0.6, 0.55));
    auto* mat_amber  = scene.add_material<Metal>(Color(0.85, 0.7, 0.25), 0.1);
    auto* mat_silver = scene.add_material<Metal>(Color(0.8, 0.82, 0.85), 0.02);
    auto* mat_glass  = scene.add_material<Dielectric>(1.5);

    scene.world.add(std::make_shared<Plane>(Point3(0, 0, 0), Vec3(0, 1, 0), mat_ground));

    // Pyramide
    Point3 a(-0.8, 0, -0.8), b(0.8, 0, -0.8), c(0.8, 0, 0.8), d(-0.8, 0, 0.8);
    Point3 apex(0, 1.6, 0);
    scene.world.add(std::make_shared<Triangle>(a, b, apex, mat_red));
    scene.world.add(std::make_shared<Triangle>(b, c, apex, mat_teal));
    scene.world.add(std::make_shared<Triangle>(c, d, apex, mat_amber));
    scene.world.add(std::make_shared<Triangle>(d, a, apex, mat_silver));
    scene.world.add(std::make_shared<Triangle>(a, c, b, mat_ground));
    scene.world.add(std::make_shared<Triangle>(a, d, c, mat_ground));

    // Sphere verre a cote
    scene.world.add(std::make_shared<Sphere>(Point3(2.2, 0.5, 0.5), 0.5, mat_glass));
    // Cylindre
    scene.world.add(std::make_shared<Cylinder>(Point3(-2.0, 0, 0.5), 0.3, 1.5, mat_silver));

    scene.camera.aspect_ratio = 16.0 / 9.0;
    scene.camera.image_width = 800;
    scene.camera.samples_per_pixel = 100;
    scene.camera.max_depth = 50;
    scene.camera.vfov = 40.0;
    scene.camera.lookfrom = Point3(3.5, 2.5, 4.5);
    scene.camera.lookat = Point3(0, 0.6, 0);
    scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.0;
    scene.camera.focus_dist = 5.0;
    scene.camera.initialize();

    return scene;
}

// ============================================================
// Scene "cornell" : boite avec 2 spheres
// ============================================================
Scene Scene::build_cornell_box() {
    Scene scene;

    auto* mat_white = scene.add_material<Lambertian>(Color(0.73, 0.73, 0.73));
    auto* mat_red   = scene.add_material<Lambertian>(Color(0.65, 0.05, 0.05));
    auto* mat_green = scene.add_material<Lambertian>(Color(0.12, 0.45, 0.15));
    auto* mat_metal = scene.add_material<Metal>(Color(0.8, 0.85, 0.88), 0.0);
    auto* mat_glass = scene.add_material<Dielectric>(1.5);
    auto* mat_light = scene.add_material<Emissive>(Color(1, 1, 1), 15.0);

    double S = 5.55; // taille de la boite
    // Sol
    scene.world.add(std::make_shared<Triangle>(Point3(0,0,0), Point3(S,0,0), Point3(S,0,S), mat_white));
    scene.world.add(std::make_shared<Triangle>(Point3(0,0,0), Point3(S,0,S), Point3(0,0,S), mat_white));
    // Plafond
    scene.world.add(std::make_shared<Triangle>(Point3(0,S,0), Point3(S,S,S), Point3(S,S,0), mat_white));
    scene.world.add(std::make_shared<Triangle>(Point3(0,S,0), Point3(0,S,S), Point3(S,S,S), mat_white));
    // Fond
    scene.world.add(std::make_shared<Triangle>(Point3(0,0,S), Point3(S,0,S), Point3(S,S,S), mat_white));
    scene.world.add(std::make_shared<Triangle>(Point3(0,0,S), Point3(S,S,S), Point3(0,S,S), mat_white));
    // Gauche rouge
    scene.world.add(std::make_shared<Triangle>(Point3(0,0,0), Point3(0,0,S), Point3(0,S,S), mat_red));
    scene.world.add(std::make_shared<Triangle>(Point3(0,0,0), Point3(0,S,S), Point3(0,S,0), mat_red));
    // Droit vert
    scene.world.add(std::make_shared<Triangle>(Point3(S,0,0), Point3(S,S,0), Point3(S,S,S), mat_green));
    scene.world.add(std::make_shared<Triangle>(Point3(S,0,0), Point3(S,S,S), Point3(S,0,S), mat_green));
    // Lumiere au plafond
    double L = 1.5;
    double cx = S/2, cz = S/2;
    scene.world.add(std::make_shared<Triangle>(
        Point3(cx-L, S-0.01, cz-L), Point3(cx+L, S-0.01, cz-L), Point3(cx+L, S-0.01, cz+L), mat_light));
    scene.world.add(std::make_shared<Triangle>(
        Point3(cx-L, S-0.01, cz-L), Point3(cx+L, S-0.01, cz+L), Point3(cx-L, S-0.01, cz+L), mat_light));

    scene.world.add(std::make_shared<Sphere>(Point3(1.8, 0.8, 1.9), 0.8, mat_metal));
    scene.world.add(std::make_shared<Sphere>(Point3(3.7, 0.6, 3.5), 0.6, mat_glass));

    scene.camera.aspect_ratio = 1.0;
    scene.camera.image_width = 600;
    scene.camera.samples_per_pixel = 300;
    scene.camera.max_depth = 50;
    scene.camera.vfov = 40.0;
    scene.camera.lookfrom = Point3(S/2, S/2, -8);
    scene.camera.lookat = Point3(S/2, S/2, 0);
    scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.0;
    scene.camera.focus_dist = 10.0;
    scene.camera.initialize();

    return scene;
}

// ============================================================
// Scene "showcase" : vitrine de toutes les features
// ============================================================
Scene Scene::build_showcase() {
    Scene scene;

    auto* mat_ground = scene.add_material<Checker>(Color(0.9, 0.9, 0.9), Color(0.3, 0.3, 0.35), 1.0);
    auto* mat_mirror   = scene.add_material<Metal>(Color(0.95, 0.95, 0.95), 0.0);
    auto* mat_gold     = scene.add_material<Metal>(Color(0.85, 0.65, 0.13), 0.05);
    auto* mat_copper   = scene.add_material<Metal>(Color(0.72, 0.45, 0.20), 0.15);
    auto* mat_glass    = scene.add_material<Dielectric>(1.5);
    auto* mat_diamond  = scene.add_material<Dielectric>(2.42);
    auto* mat_red      = scene.add_material<Lambertian>(Color(0.9, 0.15, 0.15));
    auto* mat_blue     = scene.add_material<Lambertian>(Color(0.15, 0.35, 0.9));
    auto* mat_green    = scene.add_material<Lambertian>(Color(0.2, 0.85, 0.3));
    auto* mat_orange   = scene.add_material<Lambertian>(Color(0.95, 0.55, 0.1));
    auto* mat_purple   = scene.add_material<Lambertian>(Color(0.6, 0.2, 0.8));
    auto* mat_light    = scene.add_material<Emissive>(Color(1.0, 0.95, 0.9), 4.0);

    scene.world.add(std::make_shared<Plane>(Point3(0, 0, 0), Vec3(0, 1, 0), mat_ground));

    scene.world.add(std::make_shared<Sphere>(Point3(0, 1.2, 0), 1.2, mat_mirror));
    scene.world.add(std::make_shared<Sphere>(Point3(-2.5, 0.8, 0.5), 0.8, mat_glass));
    scene.world.add(std::make_shared<Sphere>(Point3(-2.5, 0.8, 0.5), -0.75, mat_glass));
    scene.world.add(std::make_shared<Sphere>(Point3(2.5, 0.9, -0.3), 0.9, mat_gold));
    scene.world.add(std::make_shared<Sphere>(Point3(-1.0, 0.35, 2.0), 0.35, mat_red));
    scene.world.add(std::make_shared<Sphere>(Point3(-0.2, 0.25, 2.5), 0.25, mat_orange));
    scene.world.add(std::make_shared<Sphere>(Point3(0.7, 0.30, 2.2), 0.30, mat_blue));
    scene.world.add(std::make_shared<Sphere>(Point3(1.5, 0.28, 2.3), 0.28, mat_green));
    scene.world.add(std::make_shared<Sphere>(Point3(1.0, 0.5, -2.0), 0.5, mat_diamond));
    scene.world.add(std::make_shared<Sphere>(Point3(-1.5, 0.4, -1.8), 0.4, mat_copper));
    scene.world.add(std::make_shared<Sphere>(Point3(3.5, 0.35, 1.5), 0.35, mat_purple));
    scene.world.add(std::make_shared<Cylinder>(Point3(-3.5, 0, -1.0), 0.3, 2.0, mat_copper));

    scene.world.add(std::make_shared<Triangle>(Point3(-2,4,-2), Point3(2,4,-2), Point3(2,4,2), mat_light));
    scene.world.add(std::make_shared<Triangle>(Point3(-2,4,-2), Point3(2,4,2), Point3(-2,4,2), mat_light));

    scene.camera.aspect_ratio = 16.0 / 9.0;
    scene.camera.image_width = 800;
    scene.camera.samples_per_pixel = 200;
    scene.camera.max_depth = 50;
    scene.camera.vfov = 35.0;
    scene.camera.lookfrom = Point3(4, 2.5, 6);
    scene.camera.lookat = Point3(0, 0.5, 0);
    scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.4;
    scene.camera.focus_dist = 7.0;
    scene.camera.initialize();
    return scene;
}

// ============================================================
// Scene "hall" : salle avec colonnes
// ============================================================
Scene Scene::build_hall() {
    Scene scene;
    auto* mat_floor = scene.add_material<Checker>(Color(0.95, 0.95, 0.95), Color(0.15, 0.15, 0.2), 2.0);
    auto* mat_wall  = scene.add_material<Lambertian>(Color(0.85, 0.82, 0.78));
    auto* mat_chrome = scene.add_material<Metal>(Color(0.9, 0.9, 0.9), 0.02);
    auto* mat_gold  = scene.add_material<Metal>(Color(0.85, 0.65, 0.13), 0.0);
    auto* mat_glass = scene.add_material<Dielectric>(1.5);
    auto* mat_ruby  = scene.add_material<Lambertian>(Color(0.7, 0.05, 0.1));
    auto* mat_emerald = scene.add_material<Lambertian>(Color(0.05, 0.6, 0.15));
    auto* mat_light = scene.add_material<Emissive>(Color(1.0, 0.98, 0.95), 5.0);

    scene.world.add(std::make_shared<Plane>(Point3(0, 0, 0), Vec3(0, 1, 0), mat_floor));
    scene.world.add(std::make_shared<Triangle>(Point3(-20,0,-8), Point3(20,0,-8), Point3(20,10,-8), mat_wall));
    scene.world.add(std::make_shared<Triangle>(Point3(-20,0,-8), Point3(20,10,-8), Point3(-20,10,-8), mat_wall));

    for (int i = -3; i <= 3; i++) {
        scene.world.add(std::make_shared<Cylinder>(Point3(i*3.0, 0, -5), 0.25, 4.0, mat_chrome));
        scene.world.add(std::make_shared<Cylinder>(Point3(i*3.0, 0, 3), 0.25, 4.0, mat_chrome));
    }
    scene.world.add(std::make_shared<Sphere>(Point3(0, 1.5, -1), 1.5, mat_gold));
    for (int i = -2; i <= 2; i++)
        scene.world.add(std::make_shared<Sphere>(Point3(i*2.5, 0.4, 1.0), 0.4, mat_glass));
    scene.world.add(std::make_shared<Sphere>(Point3(-4, 0.3, 0), 0.3, mat_ruby));
    scene.world.add(std::make_shared<Sphere>(Point3(4, 0.3, 0), 0.3, mat_emerald));
    scene.world.add(std::make_shared<Triangle>(Point3(-3,4,-3), Point3(3,4,-3), Point3(3,4,1), mat_light));
    scene.world.add(std::make_shared<Triangle>(Point3(-3,4,-3), Point3(3,4,1), Point3(-3,4,1), mat_light));

    scene.camera.aspect_ratio = 16.0 / 9.0; scene.camera.image_width = 800;
    scene.camera.samples_per_pixel = 200; scene.camera.max_depth = 50;
    scene.camera.vfov = 50.0; scene.camera.lookfrom = Point3(0, 2.0, 8);
    scene.camera.lookat = Point3(0, 1.0, -2); scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.0; scene.camera.focus_dist = 10.0;
    scene.camera.initialize();
    return scene;
}

// ============================================================
// Scene "galaxy" : ~1000 spheres en spirale (stress test)
// ============================================================
Scene Scene::build_galaxy() {
    Scene scene;
    auto* mat_ground = scene.add_material<Checker>(Color(0.1, 0.1, 0.15), Color(0.05, 0.05, 0.08), 3.0);
    scene.world.add(std::make_shared<Plane>(Point3(0, 0, 0), Vec3(0, 1, 0), mat_ground));

    for (int arm = 0; arm < 4; arm++) {
        double arm_off = arm * 3.14159265 / 2.0;
        for (int i = 0; i < 250; i++) {
            double t = i * 0.04, r = 1.0 + t * 2.5, angle = t * 2.5 + arm_off;
            double x = r * std::cos(angle) + random_double(-0.3, 0.3);
            double z = r * std::sin(angle) + random_double(-0.3, 0.3);
            double y = 0.1 + random_double(0.0, 0.3), radius = 0.08 + random_double(0.0, 0.12);
            double mc = random_double();
            const Material* mat;
            if (mc < 0.5) mat = scene.add_material<Lambertian>(random_vec3() * random_vec3());
            else if (mc < 0.8) mat = scene.add_material<Metal>(random_vec3(0.5, 1.0), random_double(0, 0.3));
            else mat = scene.add_material<Dielectric>(1.5);
            scene.world.add(std::make_shared<Sphere>(Point3(x, y, z), radius, mat));
        }
    }
    auto* mat_c = scene.add_material<Metal>(Color(0.95, 0.95, 0.98), 0.0);
    scene.world.add(std::make_shared<Sphere>(Point3(0, 1.5, 0), 1.5, mat_c));
    scene.world.add(std::make_shared<Sphere>(Point3(-3, 0.8, 2), 0.8, scene.add_material<Dielectric>(1.5)));
    scene.world.add(std::make_shared<Sphere>(Point3(3, 0.7, -2), 0.7, scene.add_material<Metal>(Color(0.85, 0.65, 0.13), 0.0)));

    scene.camera.aspect_ratio = 16.0 / 9.0; scene.camera.image_width = 800;
    scene.camera.samples_per_pixel = 100; scene.camera.max_depth = 30;
    scene.camera.vfov = 30.0; scene.camera.lookfrom = Point3(15, 8, 15);
    scene.camera.lookat = Point3(0, 0.5, 0); scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.3; scene.camera.focus_dist = 20.0;
    scene.camera.initialize();
    return scene;
}

// ============================================================
// Scene "minimal" : noir blanc miroir verre, DOF
// ============================================================
Scene Scene::build_minimal() {
    Scene scene;
    auto* mat_white = scene.add_material<Lambertian>(Color(0.95, 0.95, 0.95));
    auto* mat_black = scene.add_material<Lambertian>(Color(0.02, 0.02, 0.02));
    auto* mat_mirror = scene.add_material<Metal>(Color(0.98, 0.98, 0.98), 0.0);
    auto* mat_glass = scene.add_material<Dielectric>(1.5);

    scene.world.add(std::make_shared<Plane>(Point3(0, 0, 0), Vec3(0, 1, 0), mat_white));
    scene.world.add(std::make_shared<Triangle>(Point3(-10,0,-5), Point3(10,0,-5), Point3(10,10,-5), mat_white));
    scene.world.add(std::make_shared<Triangle>(Point3(-10,0,-5), Point3(10,10,-5), Point3(-10,10,-5), mat_white));

    scene.world.add(std::make_shared<Sphere>(Point3(-2, 1, 0), 1.0, mat_black));
    scene.world.add(std::make_shared<Sphere>(Point3(0, 1, 0), 1.0, mat_mirror));
    scene.world.add(std::make_shared<Sphere>(Point3(2, 1, 0), 1.0, mat_glass));
    scene.world.add(std::make_shared<Sphere>(Point3(2, 1, 0), -0.95, mat_glass));

    scene.camera.aspect_ratio = 16.0 / 9.0; scene.camera.image_width = 800;
    scene.camera.samples_per_pixel = 300; scene.camera.max_depth = 50;
    scene.camera.vfov = 30.0; scene.camera.lookfrom = Point3(0, 2.5, 8);
    scene.camera.lookat = Point3(0, 0.8, 0); scene.camera.vup = Vec3(0, 1, 0);
    scene.camera.defocus_angle = 0.8; scene.camera.focus_dist = 8.0;
    scene.camera.initialize();
    return scene;
}
