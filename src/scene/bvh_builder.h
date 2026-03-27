#ifndef RT_SCENE_BVH_BUILDER_H
#define RT_SCENE_BVH_BUILDER_H

#include "scene/bvh_node.cuh"
#include <vector>
#include <memory>

class HittableList;

// Optimized flat BVH: SAH construction, iterative traversal, front-to-back order
class BVHAccel : public Hittable {
public:
    BVHAccel(const HittableList& list);

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

#endif // RT_SCENE_BVH_BUILDER_H
