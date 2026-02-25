#include "core/hittable_list.h"

void HittableList::add(std::shared_ptr<Hittable> object) {
    bbox_ = surrounding_box(bbox_, object->bounding_box());
    objects_.push_back(std::move(object));
}

void HittableList::clear() {
    objects_.clear();
}

bool HittableList::hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    double closest_so_far = ray_t.max;

    for (const auto& object : objects_) {
        if (object->hit(r, Interval(ray_t.min, closest_so_far), temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}

AABB HittableList::bounding_box() const {
    return bbox_;
}
