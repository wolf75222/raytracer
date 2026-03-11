#ifndef RT_BVH_H
#define RT_BVH_H

#include "hittable.h"
#include "aabb.h"
#include <vector>
#include <memory>
#include <cstdint>

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

// Optimized flat BVH: SAH construction, iterative traversal, front-to-back order
class BVHAccel : public Hittable {
public:
    BVHAccel(const class HittableList& list);

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override;
    AABB bounding_box() const override;

private:
    std::vector<FlatBVHNode> nodes_;
    std::vector<std::shared_ptr<Hittable>> ordered_prims_;

    struct BuildPrimInfo {
        size_t prim_index;
        AABB bbox;
        Point3 centroid;
    };

    struct BuildNode {
        AABB bbox;
        BuildNode* children[2] = {nullptr, nullptr};
        int split_axis = 0;
        int first_prim_offset = 0;
        int num_primitives = 0;
    };

    BuildNode* recursive_build(
        std::vector<BuildPrimInfo>& prim_info,
        int start, int end,
        std::vector<std::shared_ptr<Hittable>>& src_prims,
        int& total_nodes);

    int flatten_tree(BuildNode* node, int& offset);
    void delete_tree(BuildNode* node);
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

#endif // RT_BVH_H
