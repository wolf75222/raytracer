#include "core/bvh.h"
#include "core/hittable_list.h"
#include <algorithm>
#include <limits>

// ============================================================
// BVHAccel - Optimized flat BVH
// SAH construction, iterative traversal, front-to-back order
// ============================================================

BVHAccel::BVHAccel(const HittableList& list) {
    auto prims = list.objects();
    if (prims.empty()) return;

    // Build primitive info
    std::vector<BuildPrimInfo> prim_info(prims.size());
    for (size_t i = 0; i < prims.size(); ++i) {
        prim_info[i].prim_index = i;
        prim_info[i].bbox = prims[i]->bounding_box();
        prim_info[i].centroid = (prim_info[i].bbox.min_point() + prim_info[i].bbox.max_point()) * 0.5;
    }

    // Build tree (using new for stable pointers)
    int total_nodes = 0;
    BuildNode* root = recursive_build(prim_info, 0, static_cast<int>(prims.size()),
                                       prims, total_nodes);

    // Flatten to contiguous array
    nodes_.resize(total_nodes);
    int offset = 0;
    flatten_tree(root, offset);

    // Free build tree
    delete_tree(root);
}

BVHAccel::BuildNode* BVHAccel::recursive_build(
    std::vector<BuildPrimInfo>& prim_info,
    int start, int end,
    std::vector<std::shared_ptr<Hittable>>& src_prims,
    int& total_nodes)
{
    BuildNode* node = new BuildNode();
    total_nodes++;

    // Compute bounds of all primitives in range
    AABB bounds;
    for (int i = start; i < end; ++i)
        bounds = surrounding_box(bounds, prim_info[i].bbox);

    int n_prims = end - start;

    // Small leaf
    if (n_prims <= 2) {
        int first_offset = static_cast<int>(ordered_prims_.size());
        for (int i = start; i < end; ++i)
            ordered_prims_.push_back(src_prims[prim_info[i].prim_index]);
        node->bbox = bounds;
        node->first_prim_offset = first_offset;
        node->num_primitives = n_prims;
        return node;
    }

    // Compute centroid bounds
    AABB centroid_bounds;
    for (int i = start; i < end; ++i) {
        Point3 c = prim_info[i].centroid;
        centroid_bounds = surrounding_box(centroid_bounds, AABB(c, c));
    }

    int axis = centroid_bounds.longest_axis();
    double axis_extent = centroid_bounds.axis_interval(axis).size();

    // Degenerate: all centroids at same position
    if (axis_extent < 1e-8) {
        int first_offset = static_cast<int>(ordered_prims_.size());
        for (int i = start; i < end; ++i)
            ordered_prims_.push_back(src_prims[prim_info[i].prim_index]);
        node->bbox = bounds;
        node->first_prim_offset = first_offset;
        node->num_primitives = n_prims;
        return node;
    }

    // SAH bucket evaluation
    constexpr int N_BUCKETS = 12;
    struct Bucket { int count = 0; AABB bounds; };
    Bucket buckets[N_BUCKETS];

    double cmin = centroid_bounds.axis_interval(axis).min;
    double cmax = centroid_bounds.axis_interval(axis).max;

    for (int i = start; i < end; ++i) {
        int b = static_cast<int>(N_BUCKETS *
            ((prim_info[i].centroid[axis] - cmin) / (cmax - cmin)));
        if (b >= N_BUCKETS) b = N_BUCKETS - 1;
        buckets[b].count++;
        buckets[b].bounds = surrounding_box(buckets[b].bounds, prim_info[i].bbox);
    }

    // Evaluate cost for each split position
    constexpr double T_TRAV = 0.125;
    constexpr double T_ISECT = 1.0;
    double parent_area = bounds.surface_area();
    double best_cost = std::numeric_limits<double>::infinity();
    int best_split = -1;

    // Forward sweep: accumulate left side
    double cost_left_area[N_BUCKETS - 1];
    int cost_left_count[N_BUCKETS - 1];
    {
        AABB accum;
        int count = 0;
        for (int i = 0; i < N_BUCKETS - 1; ++i) {
            accum = surrounding_box(accum, buckets[i].bounds);
            count += buckets[i].count;
            cost_left_area[i] = accum.surface_area();
            cost_left_count[i] = count;
        }
    }
    // Backward sweep: accumulate right side and find best
    {
        AABB accum;
        int count = 0;
        for (int i = N_BUCKETS - 1; i >= 1; --i) {
            accum = surrounding_box(accum, buckets[i].bounds);
            count += buckets[i].count;
            double cost = T_TRAV + T_ISECT *
                (cost_left_count[i-1] * cost_left_area[i-1] +
                 count * accum.surface_area()) / parent_area;
            if (cost < best_cost) {
                best_cost = cost;
                best_split = i;
            }
        }
    }

    // Compare with leaf cost
    double leaf_cost = T_ISECT * n_prims;
    if (best_cost >= leaf_cost && n_prims <= 16) {
        // Leaf is cheaper
        int first_offset = static_cast<int>(ordered_prims_.size());
        for (int i = start; i < end; ++i)
            ordered_prims_.push_back(src_prims[prim_info[i].prim_index]);
        node->bbox = bounds;
        node->first_prim_offset = first_offset;
        node->num_primitives = n_prims;
        return node;
    }

    // Partition primitives at best split
    auto mid_it = std::partition(
        prim_info.begin() + start, prim_info.begin() + end,
        [&](const BuildPrimInfo& pi) {
            int b = static_cast<int>(N_BUCKETS *
                ((pi.centroid[axis] - cmin) / (cmax - cmin)));
            if (b >= N_BUCKETS) b = N_BUCKETS - 1;
            return b < best_split;
        });
    int mid = static_cast<int>(std::distance(prim_info.begin(), mid_it));

    if (mid == start || mid == end)
        mid = start + n_prims / 2;

    // Recurse
    node->children[0] = recursive_build(prim_info, start, mid, src_prims, total_nodes);
    node->children[1] = recursive_build(prim_info, mid, end, src_prims, total_nodes);
    node->bbox = surrounding_box(node->children[0]->bbox, node->children[1]->bbox);
    node->split_axis = axis;
    node->num_primitives = 0;
    return node;
}

int BVHAccel::flatten_tree(BuildNode* node, int& offset) {
    FlatBVHNode& flat = nodes_[offset];
    flat.bbox = node->bbox;
    int my_offset = offset++;

    if (node->num_primitives > 0) {
        // Leaf
        flat.primitives_offset = static_cast<uint32_t>(node->first_prim_offset);
        flat.num_primitives = static_cast<uint16_t>(node->num_primitives);
    } else {
        // Interior
        flat.split_axis = static_cast<uint8_t>(node->split_axis);
        flat.num_primitives = 0;
        // Left child is immediately next (offset)
        flatten_tree(node->children[0], offset);
        // Right child offset stored in node
        flat.second_child_offset = static_cast<uint32_t>(
            flatten_tree(node->children[1], offset));
    }
    return my_offset;
}

void BVHAccel::delete_tree(BuildNode* node) {
    if (!node) return;
    delete_tree(node->children[0]);
    delete_tree(node->children[1]);
    delete node;
}

bool BVHAccel::hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
    if (nodes_.empty()) return false;

    bool hit_anything = false;
    double closest = ray_t.max;

    // Iterative traversal with explicit stack
    constexpr int MAX_STACK = 64;
    int stack[MAX_STACK];
    int stack_ptr = 0;
    stack[stack_ptr++] = 0;

    while (stack_ptr > 0) {
        int node_idx = stack[--stack_ptr];
        const FlatBVHNode& node = nodes_[node_idx];

        if (!node.bbox.hit(r, ray_t.min, closest))
            continue;

        if (node.num_primitives > 0) {
            // Leaf: test all primitives
            for (uint16_t i = 0; i < node.num_primitives; ++i) {
                if (ordered_prims_[node.primitives_offset + i]->hit(
                        r, Interval(ray_t.min, closest), rec)) {
                    hit_anything = true;
                    closest = rec.t;
                }
            }
        } else {
            // Interior: push children in front-to-back order
            // Left child = node_idx + 1, right child = second_child_offset
            int near_child = node_idx + 1;
            int far_child = node.second_child_offset;

            // If ray direction is negative along split axis, swap
            if (r.dir_sign[node.split_axis]) {
                int tmp = near_child; near_child = far_child; far_child = tmp;
            }

            // Push far first (popped last = traversed last)
            stack[stack_ptr++] = far_child;
            stack[stack_ptr++] = near_child;
        }
    }
    return hit_anything;
}

AABB BVHAccel::bounding_box() const {
    if (nodes_.empty()) return AABB::empty;
    return nodes_[0].bbox;
}

// ============================================================
// Legacy BVHNode (for test backward compatibility)
// ============================================================

BVHNode::BVHNode(const HittableList& list) {
    auto objects = list.objects();
    *this = BVHNode(objects, 0, objects.size());
}

BVHNode::BVHNode(std::vector<std::shared_ptr<Hittable>>& objects, size_t start, size_t end) {
    AABB span_bbox;
    for (size_t i = start; i < end; ++i)
        span_bbox = surrounding_box(span_bbox, objects[i]->bounding_box());
    bbox_ = span_bbox;

    size_t object_span = end - start;

    if (object_span == 1) {
        left_ = right_ = objects[start];
    } else if (object_span == 2) {
        left_ = objects[start];
        right_ = objects[start + 1];
    } else {
        int axis = bbox_.longest_axis();
        split_axis_ = axis;

        std::sort(objects.begin() + start, objects.begin() + end,
            [axis](const std::shared_ptr<Hittable>& a, const std::shared_ptr<Hittable>& b) {
                return a->bounding_box().axis_interval(axis).min
                     < b->bounding_box().axis_interval(axis).min;
            });

        size_t mid = start + object_span / 2;
        left_ = std::make_shared<BVHNode>(objects, start, mid);
        right_ = std::make_shared<BVHNode>(objects, mid, end);
    }
}

bool BVHNode::hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
    if (!bbox_.hit(r, ray_t))
        return false;

    // Front-to-back based on ray direction
    Hittable* first = (r.dir_sign[split_axis_] == 0) ? left_.get() : right_.get();
    Hittable* second = (r.dir_sign[split_axis_] == 0) ? right_.get() : left_.get();

    bool hit_near = first->hit(r, ray_t, rec);
    bool hit_far = second->hit(r,
        Interval(ray_t.min, hit_near ? rec.t : ray_t.max), rec);

    return hit_near || hit_far;
}

AABB BVHNode::bounding_box() const {
    return bbox_;
}
