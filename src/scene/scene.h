#ifndef RT_SCENE_SCENE_H
#define RT_SCENE_SCENE_H

#include "scene/primitives.cuh"
#include "scene/camera.cuh"
#include "scene/material.cuh"
#include <memory>
#include <vector>

class Scene {
public:
    Camera camera;
    HittableList world;

    // Scene owns all materials
    template<typename T, typename... Args>
    T* add_material(Args&&... args) {
        auto mat = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = mat.get();
        materials_.push_back(std::move(mat));
        return ptr;
    }

    // Factory: three spheres + ground plane
    static Scene build_three_spheres();

    // Factory: final scene with many random spheres
    static Scene build_final_scene();

    // Factory: scene with triangles (pyramide + spheres)
    static Scene build_triangle_scene();

    // Factory: Cornell Box (classique)
    static Scene build_cornell_box();

    // Factory: scene showcase (lumineuse, variee)
    static Scene build_showcase();

    // Factory: colonnes de verre et metal dans une salle
    static Scene build_hall();

    // Factory: galaxie de spheres (stress test ~1000 objets)
    static Scene build_galaxy();

    // Factory: scene minimaliste noir/blanc
    static Scene build_minimal();

private:
    std::vector<std::unique_ptr<Material>> materials_;
};

#endif // RT_SCENE_SCENE_H
