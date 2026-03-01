#ifndef RT_SCENE_H
#define RT_SCENE_H

#include "hittable_list.h"
#include "camera.h"
#include "material.h"
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

private:
    std::vector<std::unique_ptr<Material>> materials_;
};

#endif // RT_SCENE_H
