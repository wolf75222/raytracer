#ifndef RT_SCENE_BVH_NODE_CUH
#define RT_SCENE_BVH_NODE_CUH

#include "math/aabb.cuh"
#include "scene/primitives.cuh"
#include <cstdint>
#include <memory>
#include <vector>

// Compact flat BVH node for cache-friendly iterative traversal
struct FlatBVHNode {
    AABB bbox;
    union {
        uint32_t primitives_offset;   // leaf: index into ordered primitives
        uint32_t second_child_offset; // interior: index of right child (left = this+1)
    };
    uint16_t num_primitives; // 0 = interior node
    uint8_t split_axis;      // for front-to-back traversal
    uint8_t pad;
};

// Legacy recursive BVH (kept for test compatibility)
class BVHNode : public Hittable {
public:
    BVHNode(std::vector<std::shared_ptr<Hittable>>& objects, size_t start, size_t end);
    BVHNode(const class HittableList& list);

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override;
    AABB bounding_box() const override;

private:
    std::shared_ptr<Hittable> left_;
    std::shared_ptr<Hittable> right_;
    AABB bbox_;
    int split_axis_ = 0;
};

#endif // RT_SCENE_BVH_NODE_CUH
