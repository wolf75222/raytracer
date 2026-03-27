#ifndef RT_GPU_TYPES_CUH
#define RT_GPU_TYPES_CUH

#include <math.h>
#include <cstdint>

// ============================================================
// GPU Vec3 (float, POD, no vtable)
// ============================================================
struct __align__(16) GVec3 {
    float x, y, z;
    float _pad; // padding to 16 bytes for coalesced 128-bit loads

    __host__ __device__ GVec3() : x(0), y(0), z(0), _pad(0) {}
    __host__ __device__ GVec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_), _pad(0) {}

    __host__ __device__ GVec3 operator+(const GVec3& v) const { return {x+v.x, y+v.y, z+v.z}; }
    __host__ __device__ GVec3 operator-(const GVec3& v) const { return {x-v.x, y-v.y, z-v.z}; }
    __host__ __device__ GVec3 operator*(float t) const { return {x*t, y*t, z*t}; }
    __host__ __device__ GVec3 operator*(const GVec3& v) const { return {x*v.x, y*v.y, z*v.z}; }
    __host__ __device__ GVec3 operator/(float t) const { float inv = 1.0f/t; return {x*inv, y*inv, z*inv}; }
    __host__ __device__ GVec3 operator-() const { return {-x, -y, -z}; }
    __host__ __device__ GVec3& operator+=(const GVec3& v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
    __host__ __device__ GVec3& operator*=(float t) { x*=t; y*=t; z*=t; return *this; }

    __host__ __device__ float length_squared() const { return x*x + y*y + z*z; }
    __host__ __device__ float length() const { return sqrtf(length_squared()); }
    __host__ __device__ bool near_zero() const {
        return fabsf(x) < 1e-6f && fabsf(y) < 1e-6f && fabsf(z) < 1e-6f;
    }
};

__host__ __device__ inline GVec3 operator*(float t, const GVec3& v) { return {t*v.x, t*v.y, t*v.z}; }
__host__ __device__ inline float dot(const GVec3& a, const GVec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
__host__ __device__ inline GVec3 cross(const GVec3& a, const GVec3& b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
__host__ __device__ inline GVec3 normalize(const GVec3& v) {
    float len_sq = v.length_squared();
    if (len_sq > 0.0f) {
        float inv_len = rsqrtf(len_sq); // rsqrtf = hardware 1/sqrt, plus rapide que sqrtf+div
        return v * inv_len;
    }
    return v;
}
__host__ __device__ inline GVec3 reflect(const GVec3& v, const GVec3& n) {
    return v - 2.0f * dot(v, n) * n;
}
__host__ __device__ inline GVec3 refract(const GVec3& uv, const GVec3& n, float etai_over_etat) {
    float cos_theta = fminf(dot(-uv, n), 1.0f);
    GVec3 r_perp = etai_over_etat * (uv + cos_theta * n);
    GVec3 r_parallel = -sqrtf(fabsf(1.0f - r_perp.length_squared())) * n;
    return r_perp + r_parallel;
}

// ============================================================
// GPU Ray (with precomputed inverse direction)
// ============================================================
struct GRay {
    GVec3 orig;
    GVec3 dir;
    GVec3 inv_dir;
    GVec3 orig_mul_inv; // precomputed -orig * inv_dir for FMA AABB test

    __host__ __device__ GRay() {}
    __host__ __device__ GRay(const GVec3& o, const GVec3& d)
        : orig(o), dir(d),
          inv_dir(1.0f/d.x, 1.0f/d.y, 1.0f/d.z),
          orig_mul_inv(-o.x/d.x, -o.y/d.y, -o.z/d.z) {}

    __host__ __device__ GVec3 at(float t) const { return orig + dir * t; }
};

// ============================================================
// GPU AABB
// ============================================================
struct GAABB {
    GVec3 mn, mx; // min, max

    __host__ __device__ GAABB() : mn(1e30f, 1e30f, 1e30f), mx(-1e30f, -1e30f, -1e30f) {}
    __host__ __device__ GAABB(const GVec3& a, const GVec3& b) : mn(a), mx(b) {}

    __host__ __device__ bool hit(const GRay& r, float t_min, float t_max) const {
        // FMA-based slab test (Opt#7 level 4 from RTIOW CUDA article)
        // fmaf(invDir, bound, origMulInv) = invDir*bound + (-orig*invDir) = (bound-orig)*invDir
        float tx0 = fmaf(r.inv_dir.x, mn.x, r.orig_mul_inv.x);
        float tx1 = fmaf(r.inv_dir.x, mx.x, r.orig_mul_inv.x);
        float ty0 = fmaf(r.inv_dir.y, mn.y, r.orig_mul_inv.y);
        float ty1 = fmaf(r.inv_dir.y, mx.y, r.orig_mul_inv.y);
        float tz0 = fmaf(r.inv_dir.z, mn.z, r.orig_mul_inv.z);
        float tz1 = fmaf(r.inv_dir.z, mx.z, r.orig_mul_inv.z);

        float tEnter = fmaxf(fmaxf(fminf(tx0,tx1), fminf(ty0,ty1)), fmaxf(fminf(tz0,tz1), t_min));
        float tExit  = fminf(fminf(fmaxf(tx0,tx1), fmaxf(ty0,ty1)), fminf(fmaxf(tz0,tz1), t_max));
        return tEnter <= tExit;
    }
};

// ============================================================
// GPU Material (tagged union, pas de vtable)
// ============================================================
enum GMaterialType : uint8_t { GMAT_LAMBERTIAN = 0, GMAT_METAL = 1, GMAT_DIELECTRIC = 2, GMAT_EMISSIVE = 3 };

struct GMaterial {
    GVec3 albedo;
    float param;           // fuzz pour metal, refraction_index pour dielectric
    GMaterialType type;

    __host__ __device__ GMaterial()
        : albedo(0.5f, 0.5f, 0.5f), param(0.0f), type(GMAT_LAMBERTIAN) {}
};

// ============================================================
// GPU Sphere
// ============================================================
struct GSphere {
    GVec3 center;
    float radius;
    int material_id;
};

// SoA layout pour spheres (acces coalescents sur GPU)
struct GSphereSoA {
    float* cx;  // center.x[]
    float* cy;  // center.y[]
    float* cz;  // center.z[]
    float* r;   // radius[]
    int* mat;   // material_id[]
    int count;
};

// ============================================================
// GPU Plane
// ============================================================
struct GPlane {
    GVec3 point;
    GVec3 normal;
    int material_id;
};

// ============================================================
// GPU HitRecord
// ============================================================
struct GHitRecord {
    GVec3 p;
    GVec3 normal;
    float t;
    int material_id;
    bool front_face;

    __host__ __device__ void set_face_normal(const GRay& r, const GVec3& outward_normal) {
        front_face = dot(r.dir, outward_normal) < 0.0f;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// ============================================================
// GPU Flat BVH Node
// ============================================================
struct GBVHNode {
    GAABB bbox;
    uint32_t data;        // leaf: primitives_offset | interior: second_child_offset
    uint16_t num_prims;   // 0 = interior
    uint8_t split_axis;
    uint8_t prim_type;    // 0 = sphere, 1 = plane (for leaves)
};

// ============================================================
// GPU Camera
// ============================================================
struct GCamera {
    GVec3 origin;
    GVec3 pixel00_loc;
    GVec3 pixel_delta_u;
    GVec3 pixel_delta_v;
    GVec3 defocus_disk_u;
    GVec3 defocus_disk_v;
    float defocus_angle;
    int image_width;
    int image_height;
    int samples_per_pixel;
    int max_depth;
};

#endif // RT_GPU_TYPES_CUH
