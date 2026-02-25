#ifndef RT_HITTABLE_LIST_H
#define RT_HITTABLE_LIST_H

#include "hittable.h"
#include <memory>
#include <vector>

class HittableList : public Hittable {
public:
    HittableList() = default;

    void add(std::shared_ptr<Hittable> object);
    void clear();

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override;
    AABB bounding_box() const override;

    const std::vector<std::shared_ptr<Hittable>>& objects() const { return objects_; }

private:
    std::vector<std::shared_ptr<Hittable>> objects_;
    AABB bbox_;
};

#endif // RT_HITTABLE_LIST_H
