// GPU Renderer : tout le code CUDA dans un seul fichier .cu
// Evite la separable compilation (problemes de linkage RDC sur MSVC)

#include "gpu/gpu_types.cuh"
#include "gpu/gpu_renderer.h"
#include "scene/primitives.cuh"
#include "scene/material.cuh"
#include <cstdio>
#include <cmath>
#include <vector>
#include <curand_kernel.h>

// ============================================================
// GPU RNG : curand (standard NVIDIA)
// ============================================================
typedef curandState GRng;

__device__ void grng_init(GRng& state, uint32_t tid, uint32_t seed) {
    curand_init(seed, tid, 0, &state);
}

__device__ float grng_float(GRng& state) {
    return curand_uniform(&state);
}

__device__ GVec3 random_in_unit_sphere(GRng& rng) {
    while (true) {
        GVec3 p(grng_float(rng)*2.0f-1.0f, grng_float(rng)*2.0f-1.0f, grng_float(rng)*2.0f-1.0f);
        if (p.length_squared() < 1.0f) return p;
    }
}

__device__ GVec3 random_unit_vector(GRng& rng) {
    return normalize(random_in_unit_sphere(rng));
}

__device__ GVec3 random_in_unit_disk(GRng& rng) {
    while (true) {
        GVec3 p(grng_float(rng)*2.0f-1.0f, grng_float(rng)*2.0f-1.0f, 0.0f);
        if (p.length_squared() < 1.0f) return p;
    }
}

// ============================================================
// Intersections
// ============================================================
__device__ bool intersect_sphere(const GSphere& s, const GRay& r, float t_min, float t_max, GHitRecord& rec) {
    GVec3 oc = r.orig - s.center;
    float a = dot(r.dir, r.dir);
    float half_b = dot(oc, r.dir);
    float c = oc.length_squared() - s.radius * s.radius;
    float disc = half_b * half_b - a * c;
    if (disc < 0.0f) return false;
    float sqrtd = sqrtf(disc);
    float inv_a = __fdividef(1.0f, a); // fast HW reciprocal, avoids full division
    float root = (-half_b - sqrtd) * inv_a;
    if (root < t_min || root > t_max) {
        root = (-half_b + sqrtd) * inv_a;
        if (root < t_min || root > t_max) return false;
    }
    rec.t = root;
    rec.p = r.at(root);
    float inv_r = __fdividef(1.0f, s.radius);
    rec.set_face_normal(r, (rec.p - s.center) * inv_r);
    rec.material_id = s.material_id;
    return true;
}

// SoA layout: coalesced loads when warp threads test consecutive spheres
__device__ bool intersect_sphere_soa(const GSphereSoA& soa, int idx,
                                      const GRay& r, float t_min, float t_max, GHitRecord& rec) {
    float sx = soa.cx[idx], sy = soa.cy[idx], sz = soa.cz[idx], sr = soa.r[idx];
    GVec3 oc(r.orig.x - sx, r.orig.y - sy, r.orig.z - sz);
    float a = dot(r.dir, r.dir);
    float half_b = dot(oc, r.dir);
    float c = oc.length_squared() - sr * sr;
    float disc = half_b * half_b - a * c;
    if (disc < 0.0f) return false;
    float sqrtd = sqrtf(disc);
    float inv_a = __fdividef(1.0f, a);
    float root = (-half_b - sqrtd) * inv_a;
    if (root < t_min || root > t_max) {
        root = (-half_b + sqrtd) * inv_a;
        if (root < t_min || root > t_max) return false;
    }
    rec.t = root;
    rec.p = r.at(root);
    float inv_r = __fdividef(1.0f, sr);
    rec.set_face_normal(r, GVec3(rec.p.x - sx, rec.p.y - sy, rec.p.z - sz) * inv_r);
    rec.material_id = soa.mat[idx];
    return true;
}

__device__ bool intersect_plane(const GPlane& pl, const GRay& r, float t_min, float t_max, GHitRecord& rec) {
    float denom = dot(r.dir, pl.normal);
    if (fabsf(denom) < 1e-6f) return false;
    float t = dot(pl.point - r.orig, pl.normal) * __fdividef(1.0f, denom);
    if (t < t_min || t > t_max) return false;
    rec.t = t;
    rec.p = r.at(t);
    rec.set_face_normal(r, pl.normal);
    rec.material_id = pl.material_id;
    return true;
}

// ============================================================
// BVH traversal iteratif (stack fixe, front-to-back)
// ============================================================
// Top BVH nodes in shared mem: accessed by nearly all rays in the block
#define BVH_SHARED_NODES 7  // 3 premiers niveaux = 1+2+4 = 7 nodes

__device__ bool trace_bvh(const GBVHNode* __restrict__ nodes, int num_nodes,
                          const GSphereSoA& soa,
                          const GPlane* __restrict__ planes,
                          const GRay& r, float t_min, float t_max,
                          GHitRecord& rec,
                          const GBVHNode* __restrict__ sh_bvh, int sh_count) {
    bool hit_anything = false;
    float closest = t_max;
    int stack[16];
    int sp = 0;
    stack[sp++] = 0;

    while (sp > 0) {
        int idx = stack[--sp];
        const GBVHNode& node = (idx < sh_count) ? sh_bvh[idx] : nodes[idx];

        if (!node.bbox.hit(r, t_min, closest))
            continue;

        if (node.num_prims > 0) {
            #pragma unroll 4
            for (uint16_t i = 0; i < node.num_prims; i++) {
                GHitRecord tmp;
                // SoA: each float[] is contiguous -> coalesced reads
                if (intersect_sphere_soa(soa, node.data + i, r, t_min, closest, tmp)) {
                    hit_anything = true; closest = tmp.t; rec = tmp;
                }
            }
        } else {
            int near_child = idx + 1;
            int far_child = node.data;
            float dir_comp = (node.split_axis == 0) ? r.dir.x :
                             (node.split_axis == 1) ? r.dir.y : r.dir.z;
            if (dir_comp < 0.0f) { int tmp = near_child; near_child = far_child; far_child = tmp; }
            stack[sp++] = far_child;
            stack[sp++] = near_child;
        }
    }
    return hit_anything;
}

// Brute force fallback
__device__ bool trace_brute(const GSphere* spheres, int num_spheres,
                            const GPlane* planes, int num_planes,
                            const GRay& r, float t_min, float t_max,
                            GHitRecord& rec) {
    bool hit = false;
    float closest = t_max;
    GHitRecord tmp;
    for (int i = 0; i < num_spheres; i++) {
        if (intersect_sphere(spheres[i], r, t_min, closest, tmp)) {
            hit = true; closest = tmp.t; rec = tmp;
        }
    }
    for (int i = 0; i < num_planes; i++) {
        if (intersect_plane(planes[i], r, t_min, closest, tmp)) {
            hit = true; closest = tmp.t; rec = tmp;
        }
    }
    return hit;
}

// ============================================================
// Material scatter
// ============================================================
__device__ bool scatter_mat(const GMaterial& mat, const GRay& r_in, const GHitRecord& rec,
                            GVec3& attenuation, GRay& scattered, GRng& rng) {
    switch (mat.type) {
    case GMAT_LAMBERTIAN: {
        GVec3 dir = rec.normal + random_unit_vector(rng);
        if (dir.near_zero()) dir = rec.normal;
        scattered = GRay(rec.p, dir);
        attenuation = mat.albedo;
        return true;
    }
    case GMAT_METAL: {
        GVec3 reflected = reflect(normalize(r_in.dir), rec.normal);
        reflected = reflected + mat.param * random_in_unit_sphere(rng);
        scattered = GRay(rec.p, reflected);
        attenuation = mat.albedo;
        return dot(scattered.dir, rec.normal) > 0.0f;
    }
    case GMAT_DIELECTRIC: {
        attenuation = mat.albedo; // tinted glass support
        float ri = rec.front_face ? (1.0f / mat.param) : mat.param;
        GVec3 unit_dir = normalize(r_in.dir);
        float cos_theta = fminf(dot(-unit_dir, rec.normal), 1.0f);
        float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);
        float r0 = (1.0f - ri) / (1.0f + ri); r0 = r0 * r0;
        // manual pow5: avoids expensive powf (exp+log chain)
        float omc = 1.0f - cos_theta;
        float omc2 = omc * omc;
        float schlick = r0 + (1.0f - r0) * omc2 * omc2 * omc;
        GVec3 direction;
        if (ri * sin_theta > 1.0f || schlick > grng_float(rng))
            direction = reflect(unit_dir, rec.normal);
        else
            direction = refract(unit_dir, rec.normal, ri);
        scattered = GRay(rec.p, direction);
        return true;
    }
    }
    return false;
}

// ============================================================
// Ray color iteratif
// ============================================================
__device__ GVec3 ray_color(GRay r,
                           const GBVHNode* bvh, int num_bvh,
                           const GSphereSoA& soa,
                           const GSphere* spheres, int num_spheres,
                           const GPlane* planes, int num_planes,
                           const GMaterial* materials,
                           int max_depth, GRng& rng,
                           const GBVHNode* sh_bvh, int sh_count) {
    GVec3 throughput(1.0f, 1.0f, 1.0f);

    for (int d = 0; d < max_depth; d++) {
        GHitRecord rec;
        bool hit;
        if (num_bvh > 0) {
            // BVH pour les spheres + brute force pour les plans (peu nombreux)
            float closest = 1e30f;
            hit = trace_bvh(bvh, num_bvh, soa, planes, r, 0.001f, closest, rec, sh_bvh, sh_count);
            if (hit) closest = rec.t;
            // Tester les plans en brute force
            GHitRecord tmp;
            for (int i = 0; i < num_planes; i++) {
                if (intersect_plane(planes[i], r, 0.001f, closest, tmp)) {
                    hit = true; closest = tmp.t; rec = tmp;
                }
            }
        } else {
            hit = trace_brute(spheres, num_spheres, planes, num_planes, r, 0.001f, 1e30f, rec);
        }
        if (!hit) {
            GVec3 unit_dir = normalize(r.dir);
            float a = 0.5f * (unit_dir.y + 1.0f);
            GVec3 sky(1.0f - 0.5f*a, 1.0f - 0.3f*a, 1.0f);
            return throughput * sky;
        }
        const GMaterial& mat = materials[rec.material_id];
        if (mat.type == GMAT_EMISSIVE)
            return throughput * mat.albedo * mat.param;

        GVec3 attenuation;
        GRay scattered;
        if (!scatter_mat(mat, r, rec, attenuation, scattered, rng))
            return GVec3(0, 0, 0);
        throughput = throughput * attenuation;

        // Russian roulette: unbiased path termination after depth 3
        float max_comp = fmaxf(throughput.x, fmaxf(throughput.y, throughput.z));
        if (max_comp < 0.001f)
            break;

        if (d > 3) {
            if (grng_float(rng) > max_comp)
                break;
            throughput = throughput * (1.0f / max_comp);
        }

        r = scattered;
    }
    return GVec3(0, 0, 0);
}

// ============================================================
// Kernel : 1 thread = 1 pixel, blocs 16x16
// ============================================================
// Camera in constant memory: broadcast to all threads, dedicated cache
// Stored as raw bytes to avoid dynamic initialization issues
__constant__ char d_camera_bytes[sizeof(GCamera)];
#define d_camera (*(const GCamera*)d_camera_bytes)

__global__ void render_kernel(float* __restrict__ framebuffer, int width, int height,
                              const GBVHNode* __restrict__ bvh, int num_bvh,
                              const GSphere* __restrict__ spheres, int num_spheres,
                              const GPlane* __restrict__ planes, int num_planes,
                              const GMaterial* __restrict__ materials,
                              const float* __restrict__ soa_cx, const float* __restrict__ soa_cy,
                              const float* __restrict__ soa_cz, const float* __restrict__ soa_r,
                              const int* __restrict__ soa_mat) {
    // Cooperative load of top BVH nodes into shared mem
    __shared__ GBVHNode sh_bvh[BVH_SHARED_NODES];
    int tid_flat = threadIdx.y * blockDim.x + threadIdx.x;
    if (tid_flat < BVH_SHARED_NODES && tid_flat < num_bvh) {
        sh_bvh[tid_flat] = bvh[tid_flat];
    }
    __syncthreads(); // Attendre que tous les top nodes soient en shared mem

    int sh_count = (num_bvh < BVH_SHARED_NODES) ? num_bvh : BVH_SHARED_NODES;

    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    GRng rng;
    grng_init(rng, y * width + x, 42);

    GVec3 pixel_color(0, 0, 0);
    int spp = d_camera.samples_per_pixel;
    float inv_spp = 1.0f / (float)spp;

    for (int s = 0; s < spp; s++) {
        float u_off = grng_float(rng) - 0.5f;
        float v_off = grng_float(rng) - 0.5f;
        GVec3 pixel_sample = d_camera.pixel00_loc
                            + ((float)x + u_off) * d_camera.pixel_delta_u
                            + ((float)y + v_off) * d_camera.pixel_delta_v;
        GVec3 ray_origin = d_camera.origin;
        if (d_camera.defocus_angle > 0.0f) {
            GVec3 pd = random_in_unit_disk(rng);
            ray_origin = d_camera.origin + pd.x * d_camera.defocus_disk_u + pd.y * d_camera.defocus_disk_v;
        }
        GRay r(ray_origin, pixel_sample - ray_origin);
        GSphereSoA soa;
        soa.cx = const_cast<float*>(soa_cx); soa.cy = const_cast<float*>(soa_cy);
        soa.cz = const_cast<float*>(soa_cz); soa.r = const_cast<float*>(soa_r);
        soa.mat = const_cast<int*>(soa_mat); soa.count = num_spheres;
        pixel_color += ray_color(r, bvh, num_bvh, soa, spheres, num_spheres, planes, num_planes,
                                 materials, d_camera.max_depth, rng, sh_bvh, sh_count);
    }

    pixel_color *= inv_spp;

    // Planar layout (RRR...GGG...BBB): stride-1 writes for coalescing
    int pixel_idx = y * width + x;
    int total_pixels = width * height;
    // Linear HDR output, gamma/tonemap done later
    framebuffer[pixel_idx]                  = fmaxf(pixel_color.x, 0.0f);
    framebuffer[pixel_idx + total_pixels]   = fmaxf(pixel_color.y, 0.0f);
    framebuffer[pixel_idx + total_pixels*2] = fmaxf(pixel_color.z, 0.0f);
}

// ============================================================
// Tonemap GPU kernel : lineaire -> ACES -> gamma -> ARGB 8-bit
// Evite le transfert float D->H + tonemap CPU
// ============================================================
__device__ float aces_tonemap_gpu(float x) {
    float a = x * (2.51f * x + 0.03f);
    float b = x * (2.43f * x + 0.59f) + 0.14f;
    return fmaxf(0.0f, fminf(1.0f, a / b));
}

__global__ void tonemap_kernel(const float* __restrict__ linear_fb,
                               unsigned char* __restrict__ argb_out,
                               int width, int height, int total_samples) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    int px = y * width + x;
    int tp = width * height;
    float inv_n = 1.0f / (float)total_samples;

    float r = sqrtf(aces_tonemap_gpu(fmaxf(linear_fb[px] * inv_n, 0.0f)));
    float g = sqrtf(aces_tonemap_gpu(fmaxf(linear_fb[px + tp] * inv_n, 0.0f)));
    float b = sqrtf(aces_tonemap_gpu(fmaxf(linear_fb[px + tp*2] * inv_n, 0.0f)));

    int idx = px * 4;
    argb_out[idx + 0] = (unsigned char)(fminf(b, 1.0f) * 255.0f);
    argb_out[idx + 1] = (unsigned char)(fminf(g, 1.0f) * 255.0f);
    argb_out[idx + 2] = (unsigned char)(fminf(r, 1.0f) * 255.0f);
    argb_out[idx + 3] = 255;
}

// ============================================================
// Accumulation kernel : ajoute le rendu au buffer d'accumulation
// ============================================================
__global__ void accumulate_kernel(const float* __restrict__ frame_fb,
                                  float* __restrict__ accum_fb,
                                  int total_pixels) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= total_pixels * 3) return;
    accum_fb[idx] += frame_fb[idx];
}

// ============================================================
// GPU Denoise kernel : filtre bilateral 3x3 sur le framebuffer lineaire
// Lisse le bruit en preservant les contours (difference de couleur)
// ============================================================
__global__ void denoise_bilateral_kernel(
    const float* __restrict__ input,
    float* __restrict__ output,
    int width, int height, float sigma_color)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;

    int tp = width * height;
    int px = y * width + x;
    float cr = input[px], cg = input[px+tp], cb = input[px+tp*2];

    float sum_r = 0, sum_g = 0, sum_b = 0, wsum = 0;
    float inv_sigma2 = 1.0f / (2.0f * sigma_color * sigma_color);

    for (int dy = -1; dy <= 1; dy++) {
        int ny = y + dy;
        if (ny < 0 || ny >= height) continue;
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx;
            if (nx < 0 || nx >= width) continue;
            int npx = ny * width + nx;
            float nr = input[npx], ng = input[npx+tp], nb = input[npx+tp*2];
            float dr = cr-nr, dg = cg-ng, db = cb-nb;
            float color_dist = dr*dr + dg*dg + db*db;
            float w = expf(-color_dist * inv_sigma2);
            sum_r += nr * w; sum_g += ng * w; sum_b += nb * w;
            wsum += w;
        }
    }
    float inv_w = 1.0f / (wsum + 1e-8f);
    output[px] = sum_r * inv_w;
    output[px+tp] = sum_g * inv_w;
    output[px+tp*2] = sum_b * inv_w;
}

// ============================================================
// GpuRenderer host code
// ============================================================
#define CUDA_CHECK(call) do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
        return; \
    } \
} while(0)

GpuRenderer::GpuRenderer() {}
GpuRenderer::~GpuRenderer() { free_scene(); free_framebuffer(); }

void GpuRenderer::free_scene() {
    if (d_spheres_) { cudaFree(d_spheres_); d_spheres_ = nullptr; }
    if (d_soa_cx_) { cudaFree(d_soa_cx_); d_soa_cx_ = nullptr; }
    if (d_soa_cy_) { cudaFree(d_soa_cy_); d_soa_cy_ = nullptr; }
    if (d_soa_cz_) { cudaFree(d_soa_cz_); d_soa_cz_ = nullptr; }
    if (d_soa_r_) { cudaFree(d_soa_r_); d_soa_r_ = nullptr; }
    if (d_soa_mat_) { cudaFree(d_soa_mat_); d_soa_mat_ = nullptr; }
    if (d_planes_) { cudaFree(d_planes_); d_planes_ = nullptr; }
    if (d_materials_) { cudaFree(d_materials_); d_materials_ = nullptr; }
    if (d_bvh_nodes_) { cudaFree(d_bvh_nodes_); d_bvh_nodes_ = nullptr; }
}

void GpuRenderer::free_framebuffer() {
    if (d_framebuffer_) { cudaFree(d_framebuffer_); d_framebuffer_ = nullptr; }
    fb_width_ = fb_height_ = 0;
}

void GpuRenderer::ensure_framebuffer(int width, int height) {
    if (fb_width_ == width && fb_height_ == height && d_framebuffer_) return;
    free_framebuffer();
    fb_width_ = width; fb_height_ = height;
    cudaMalloc(&d_framebuffer_, width * height * 3 * sizeof(float));
}

void GpuRenderer::upload_scene(const GSphere* spheres, int num_spheres,
                                const GPlane* planes, int num_planes,
                                const GMaterial* materials, int num_materials,
                                const GBVHNode* bvh_nodes, int num_bvh_nodes) {
    free_scene();
    num_spheres_ = num_spheres;
    num_planes_ = num_planes;
    num_materials_ = num_materials;
    num_bvh_nodes_ = num_bvh_nodes;

    if (num_spheres > 0) {
        CUDA_CHECK(cudaMalloc(&d_spheres_, num_spheres * sizeof(GSphere)));
        CUDA_CHECK(cudaMemcpy(d_spheres_, spheres, num_spheres * sizeof(GSphere), cudaMemcpyHostToDevice));

        // SoA upload: split fields for coalesced access in kernel
        std::vector<float> cx(num_spheres), cy(num_spheres), cz(num_spheres), radii(num_spheres);
        std::vector<int> mats(num_spheres);
        for (int i = 0; i < num_spheres; i++) {
            cx[i] = spheres[i].center.x; cy[i] = spheres[i].center.y; cz[i] = spheres[i].center.z;
            radii[i] = spheres[i].radius; mats[i] = spheres[i].material_id;
        }
        CUDA_CHECK(cudaMalloc(&d_soa_cx_, num_spheres * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&d_soa_cy_, num_spheres * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&d_soa_cz_, num_spheres * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&d_soa_r_, num_spheres * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&d_soa_mat_, num_spheres * sizeof(int)));
        CUDA_CHECK(cudaMemcpy(d_soa_cx_, cx.data(), num_spheres * sizeof(float), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_soa_cy_, cy.data(), num_spheres * sizeof(float), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_soa_cz_, cz.data(), num_spheres * sizeof(float), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_soa_r_, radii.data(), num_spheres * sizeof(float), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_soa_mat_, mats.data(), num_spheres * sizeof(int), cudaMemcpyHostToDevice));
    }
    if (num_planes > 0) {
        CUDA_CHECK(cudaMalloc(&d_planes_, num_planes * sizeof(GPlane)));
        CUDA_CHECK(cudaMemcpy(d_planes_, planes, num_planes * sizeof(GPlane), cudaMemcpyHostToDevice));
    }
    if (num_materials > 0) {
        CUDA_CHECK(cudaMalloc(&d_materials_, num_materials * sizeof(GMaterial)));
        CUDA_CHECK(cudaMemcpy(d_materials_, materials, num_materials * sizeof(GMaterial), cudaMemcpyHostToDevice));
    }
    if (num_bvh_nodes > 0) {
        CUDA_CHECK(cudaMalloc(&d_bvh_nodes_, num_bvh_nodes * sizeof(GBVHNode)));
        CUDA_CHECK(cudaMemcpy(d_bvh_nodes_, bvh_nodes, num_bvh_nodes * sizeof(GBVHNode), cudaMemcpyHostToDevice));

        // L2 cache persistence pour BVH (Ampere CC 8.6+)
        // Les top nodes sont accedes par quasi tous les rayons
        size_t bvh_bytes = num_bvh_nodes * sizeof(GBVHNode);
        cudaDeviceSetLimit(cudaLimitPersistingL2CacheSize, bvh_bytes);
    }
}

void GpuRenderer::render(const GCamera& camera, float* h_output, int width, int height) {
    ensure_framebuffer(width, height);

    // Persistent stream for kernel/transfer overlap
    static cudaStream_t s_stream = nullptr;
    if (!s_stream) {
        cudaStreamCreate(&s_stream);

        // L2 cache persistence : BVH nodes en persistent L2 (Ampere CC 8.6+)
        if (d_bvh_nodes_ && num_bvh_nodes_ > 0) {
            cudaStreamAttrValue attr = {};
            attr.accessPolicyWindow.base_ptr = d_bvh_nodes_;
            attr.accessPolicyWindow.num_bytes = num_bvh_nodes_ * sizeof(GBVHNode);
            attr.accessPolicyWindow.hitRatio = 1.0f;
            attr.accessPolicyWindow.hitProp = cudaAccessPropertyPersisting;
            attr.accessPolicyWindow.missProp = cudaAccessPropertyStreaming;
            cudaStreamSetAttribute(s_stream, cudaStreamAttributeAccessPolicyWindow, &attr);
        }
    }

    // Upload camera en constant memory
    cudaMemcpyToSymbol(d_camera_bytes, &camera, sizeof(GCamera));

    // Block size via occupancy API
    int flat_block_size = 0, min_grid = 0;
    cudaOccupancyMaxPotentialBlockSize(&min_grid, &flat_block_size, render_kernel, 0, 0);
    int bx = 16, by = flat_block_size / bx;
    if (by < 1) by = 1;
    if (by > 16) by = 16;
    dim3 block_size(bx, by);
    dim3 grid_size((width + block_size.x - 1) / block_size.x,
                   (height + block_size.y - 1) / block_size.y);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start, s_stream);

    render_kernel<<<grid_size, block_size, 0, s_stream>>>(
        d_framebuffer_, width, height,
        d_bvh_nodes_, num_bvh_nodes_,
        d_spheres_, num_spheres_,
        d_planes_, num_planes_,
        d_materials_,
        d_soa_cx_, d_soa_cy_, d_soa_cz_, d_soa_r_, d_soa_mat_
    );

    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "Kernel error: %s\n", cudaGetErrorString(err));
        cudaEventDestroy(start); cudaEventDestroy(stop);
        return;
    }

    // Transfert async D->H sur le meme stream (sequentialise apres le kernel)
    // h_output DOIT etre en memoire pinned (cudaMallocHost) pour l'async
    cudaMemcpyAsync(h_output, d_framebuffer_, width * height * 3 * sizeof(float),
                    cudaMemcpyDeviceToHost, s_stream);

    cudaEventRecord(stop, s_stream);
    // Synchroniser le stream (attendre kernel + transfert)
    cudaStreamSynchronize(s_stream);

    cudaEventElapsedTime(&last_ms_, start, stop);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
}

// ============================================================
// Bridge: conversion CPU Scene -> GPU + rendu
// ============================================================
static void convert_scene(const Scene& scene,
                           std::vector<GSphere>& out_spheres,
                           std::vector<GPlane>& out_planes,
                           std::vector<GMaterial>& out_materials,
                           GCamera& out_camera) {
    std::vector<const Material*> mat_ptrs;
    auto get_mat = [&](const Material* m) -> int {
        for (int i = 0; i < (int)mat_ptrs.size(); i++)
            if (mat_ptrs[i] == m) return i;
        int idx = (int)mat_ptrs.size();
        mat_ptrs.push_back(m);
        GMaterial gm;
        switch (m->type()) {
        case Material::Type::Lambertian: {
            auto* lam = static_cast<const Lambertian*>(m);
            gm.type = GMAT_LAMBERTIAN;
            gm.albedo = GVec3((float)lam->albedo().x(), (float)lam->albedo().y(), (float)lam->albedo().z());
            break;
        }
        case Material::Type::Metal: {
            auto* met = static_cast<const Metal*>(m);
            gm.type = GMAT_METAL;
            gm.albedo = GVec3((float)met->albedo().x(), (float)met->albedo().y(), (float)met->albedo().z());
            gm.param = (float)met->fuzz();
            break;
        }
        case Material::Type::Dielectric: {
            auto* die = static_cast<const Dielectric*>(m);
            gm.type = GMAT_DIELECTRIC;
            gm.albedo = GVec3(1, 1, 1);
            gm.param = (float)die->refraction_index();
            break;
        }
        case Material::Type::Emissive: {
            auto* em = static_cast<const Emissive*>(m);
            gm.type = GMAT_EMISSIVE;
            gm.albedo = GVec3((float)em->emit_color().x(), (float)em->emit_color().y(), (float)em->emit_color().z());
            gm.param = (float)em->intensity();
            break;
        }
        default: gm.type = GMAT_LAMBERTIAN; gm.albedo = GVec3(0.5f, 0.5f, 0.5f); break;
        }
        out_materials.push_back(gm);
        return idx;
    };

    for (const auto& obj : scene.world.objects()) {
        if (auto* sp = dynamic_cast<const Sphere*>(obj.get())) {
            GSphere gs;
            gs.center = GVec3((float)sp->center().x(), (float)sp->center().y(), (float)sp->center().z());
            gs.radius = (float)sp->radius();
            HitRecord rec;
            Ray test_ray(sp->center() + Vec3(0, sp->radius() + 1, 0), Vec3(0, -1, 0));
            gs.material_id = (sp->hit(test_ray, Interval(0.001, 1000.0), rec) && rec.material)
                           ? get_mat(rec.material) : 0;
            out_spheres.push_back(gs);
        } else if (auto* pl = dynamic_cast<const Plane*>(obj.get())) {
            GPlane gp;
            HitRecord rec;
            Ray test_ray(Point3(0, 100, 0), Vec3(0, -1, 0));
            if (pl->hit(test_ray, Interval(0.001, 10000.0), rec) && rec.material) {
                gp.point = GVec3((float)rec.p.x(), (float)rec.p.y(), (float)rec.p.z());
                gp.normal = GVec3((float)rec.normal.x(), (float)rec.normal.y(), (float)rec.normal.z());
                gp.material_id = get_mat(rec.material);
            }
            out_planes.push_back(gp);
        } else {
            static bool warned = false;
            if (!warned) {
                fprintf(stderr, "Warning: GPU ne supporte que Sphere/Plane. Triangles/Cylindres ignores.\n");
                fprintf(stderr, "         Utilisez --scene showcase, final, ou three_spheres pour le GPU.\n");
                warned = true;
            }
        }
    }

    const Camera& cam = scene.camera;
    int w = cam.get_image_width(), h = cam.get_image_height();
    double theta = cam.vfov * 3.14159265358979323846 / 180.0;
    double vp_h = 2.0 * std::tan(theta / 2.0) * cam.focus_dist;
    double vp_w = vp_h * ((double)w / h);
    Vec3 wd = unit_vector(cam.lookfrom - cam.lookat);
    Vec3 ud = unit_vector(cross(cam.vup, wd));
    Vec3 vd = cross(wd, ud);
    Vec3 vu = vp_w * ud, vv = vp_h * (-vd);
    Vec3 pdu = vu / (double)w, pdv = vv / (double)h;
    Point3 p00 = cam.lookfrom - cam.focus_dist * wd - vu/2.0 - vv/2.0 + 0.5*(pdu+pdv);
    double dr = cam.focus_dist * std::tan((cam.defocus_angle/2.0)*3.14159265358979323846/180.0);

    out_camera.origin = GVec3((float)cam.lookfrom.x(), (float)cam.lookfrom.y(), (float)cam.lookfrom.z());
    out_camera.pixel00_loc = GVec3((float)p00.x(), (float)p00.y(), (float)p00.z());
    out_camera.pixel_delta_u = GVec3((float)pdu.x(), (float)pdu.y(), (float)pdu.z());
    out_camera.pixel_delta_v = GVec3((float)pdv.x(), (float)pdv.y(), (float)pdv.z());
    out_camera.defocus_angle = (float)cam.defocus_angle;
    out_camera.defocus_disk_u = GVec3((float)(ud.x()*dr), (float)(ud.y()*dr), (float)(ud.z()*dr));
    out_camera.defocus_disk_v = GVec3((float)(vd.x()*dr), (float)(vd.y()*dr), (float)(vd.z()*dr));
    out_camera.image_width = w; out_camera.image_height = h;
    out_camera.samples_per_pixel = cam.samples_per_pixel;
    out_camera.max_depth = cam.max_depth;
}

// Camera-only conversion (pas de re-conversion des objets/materiaux)
static void convert_camera_only(const Camera& cam, GCamera& out) {
    int w = cam.get_image_width(), h = cam.get_image_height();
    double theta = cam.vfov * 3.14159265358979323846 / 180.0;
    double vp_h = 2.0 * std::tan(theta / 2.0) * cam.focus_dist;
    double vp_w = vp_h * ((double)w / h);
    Vec3 wd = unit_vector(cam.lookfrom - cam.lookat);
    Vec3 ud = unit_vector(cross(cam.vup, wd));
    Vec3 vd = cross(wd, ud);
    Vec3 vu = vp_w * ud, vv = vp_h * (-vd);
    Vec3 pdu = vu / (double)w, pdv = vv / (double)h;
    Point3 p00 = cam.lookfrom - cam.focus_dist * wd - vu/2.0 - vv/2.0 + 0.5*(pdu+pdv);
    double dr = cam.focus_dist * std::tan((cam.defocus_angle/2.0)*3.14159265358979323846/180.0);

    out.origin = GVec3((float)cam.lookfrom.x(), (float)cam.lookfrom.y(), (float)cam.lookfrom.z());
    out.pixel00_loc = GVec3((float)p00.x(), (float)p00.y(), (float)p00.z());
    out.pixel_delta_u = GVec3((float)pdu.x(), (float)pdu.y(), (float)pdu.z());
    out.pixel_delta_v = GVec3((float)pdv.x(), (float)pdv.y(), (float)pdv.z());
    out.defocus_angle = (float)cam.defocus_angle;
    out.defocus_disk_u = GVec3((float)(ud.x()*dr), (float)(ud.y()*dr), (float)(ud.z()*dr));
    out.defocus_disk_v = GVec3((float)(vd.x()*dr), (float)(vd.y()*dr), (float)(vd.z()*dr));
    out.image_width = w; out.image_height = h;
    out.samples_per_pixel = cam.samples_per_pixel;
    out.max_depth = cam.max_depth;
}

// ============================================================
// Simple GPU BVH builder (on spheres only, planes are tested brute-force in leaves)
// ============================================================
struct GBuildInfo { int sphere_idx; GAABB bbox; GVec3 centroid; };

static void build_gpu_bvh(std::vector<GBuildInfo>& info, int start, int end,
                           std::vector<GBVHNode>& nodes, int num_planes) {
    int my_idx = (int)nodes.size();
    nodes.push_back(GBVHNode());

    GAABB bounds;
    for (int i = start; i < end; i++) {
        bounds.mn.x = fminf(bounds.mn.x, info[i].bbox.mn.x);
        bounds.mn.y = fminf(bounds.mn.y, info[i].bbox.mn.y);
        bounds.mn.z = fminf(bounds.mn.z, info[i].bbox.mn.z);
        bounds.mx.x = fmaxf(bounds.mx.x, info[i].bbox.mx.x);
        bounds.mx.y = fmaxf(bounds.mx.y, info[i].bbox.mx.y);
        bounds.mx.z = fmaxf(bounds.mx.z, info[i].bbox.mx.z);
    }
    nodes[my_idx].bbox = bounds;

    int n = end - start;
    if (n <= 4) {
        // Leaf : stores sphere indices [start, end)
        nodes[my_idx].data = (uint32_t)start;
        nodes[my_idx].num_prims = (uint16_t)n;
        nodes[my_idx].split_axis = 0;
        nodes[my_idx].prim_type = 0; // spheres
        return;
    }

    // Find longest axis of centroid bounds
    GVec3 cmin(1e30f,1e30f,1e30f), cmax(-1e30f,-1e30f,-1e30f);
    for (int i = start; i < end; i++) {
        cmin.x = fminf(cmin.x, info[i].centroid.x);
        cmin.y = fminf(cmin.y, info[i].centroid.y);
        cmin.z = fminf(cmin.z, info[i].centroid.z);
        cmax.x = fmaxf(cmax.x, info[i].centroid.x);
        cmax.y = fmaxf(cmax.y, info[i].centroid.y);
        cmax.z = fmaxf(cmax.z, info[i].centroid.z);
    }
    float dx = cmax.x-cmin.x, dy = cmax.y-cmin.y, dz = cmax.z-cmin.z;
    int axis = (dx > dy && dx > dz) ? 0 : (dy > dz) ? 1 : 2;

    // Sort by centroid on axis
    std::sort(info.begin()+start, info.begin()+end,
        [axis](const GBuildInfo& a, const GBuildInfo& b) {
            float va = (axis==0)?a.centroid.x:(axis==1)?a.centroid.y:a.centroid.z;
            float vb = (axis==0)?b.centroid.x:(axis==1)?b.centroid.y:b.centroid.z;
            return va < vb;
        });

    int mid = start + n / 2;

    nodes[my_idx].split_axis = (uint8_t)axis;
    nodes[my_idx].num_prims = 0; // interior
    nodes[my_idx].prim_type = 0;

    // Left child (immediately after)
    build_gpu_bvh(info, start, mid, nodes, num_planes);
    // Right child
    nodes[my_idx].data = (uint32_t)nodes.size();
    build_gpu_bvh(info, mid, end, nodes, num_planes);
}

// Renderer global persistant
GpuRenderer* s_renderer = nullptr;

void gpu_render_scene(const Scene& scene, ImageBuffer& image, float& kernel_ms) {
    // Tout est cache entre les appels : scene, BVH, renderer, framebuffer pinned
    // Renderer global pour acces depuis gpu_realtime_render_frame
    static const Scene* s_last_scene = nullptr;
    static float* s_fb = nullptr;
    static int s_fb_w = 0, s_fb_h = 0;

    GCamera gcam;

    // Re-upload scene seulement si elle a change
    if (!s_renderer || s_last_scene != &scene) {
        std::vector<GSphere> spheres;
        std::vector<GPlane> planes;
        std::vector<GMaterial> materials;
        convert_scene(scene, spheres, planes, materials, gcam);

        std::vector<GBVHNode> bvh_nodes;
        if (spheres.size() > 8) {
            std::vector<GBuildInfo> info(spheres.size());
            for (int i = 0; i < (int)spheres.size(); i++) {
                info[i].sphere_idx = i;
                float r = spheres[i].radius;
                GVec3 c = spheres[i].center;
                info[i].bbox = GAABB(GVec3(c.x-r, c.y-r, c.z-r), GVec3(c.x+r, c.y+r, c.z+r));
                info[i].centroid = c;
            }
            build_gpu_bvh(info, 0, (int)info.size(), bvh_nodes, (int)planes.size());
            std::vector<GSphere> ordered(spheres.size());
            for (int i = 0; i < (int)info.size(); i++)
                ordered[i] = spheres[info[i].sphere_idx];
            spheres = std::move(ordered);
            fprintf(stderr, "GPU BVH: %d nodes for %d spheres\n", (int)bvh_nodes.size(), (int)spheres.size());
        }

        delete s_renderer;
        s_renderer = new GpuRenderer();
        s_renderer->upload_scene(spheres.data(), (int)spheres.size(),
                              planes.data(), (int)planes.size(),
                              materials.data(), (int)materials.size(),
                              bvh_nodes.empty() ? nullptr : bvh_nodes.data(),
                              (int)bvh_nodes.size());
        s_last_scene = &scene;
    } else {
        // Scene deja uploadee : recalculer seulement la camera
        convert_camera_only(scene.camera, gcam);
    }

    int w = gcam.image_width, h = gcam.image_height;

    // Framebuffer pinned persistant
    if (!s_fb || s_fb_w != w || s_fb_h != h) {
        if (s_fb) cudaFreeHost(s_fb);
        cudaMallocHost(&s_fb, w * h * 3 * sizeof(float));
        s_fb_w = w; s_fb_h = h;
    }

    s_renderer->render(gcam, s_fb, w, h);
    kernel_ms = s_renderer->last_render_ms();

    // Copie rapide vers ImageBuffer
    int total_px = w * h;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int px = y * w + x;
            image.set_pixel(x, y, Color(
                fminf(s_fb[px], 1.0f),
                fminf(s_fb[px + total_px], 1.0f),
                fminf(s_fb[px + total_px*2], 1.0f)));
        }
}

// ============================================================
// GPU render + bilateral denoise
void gpu_render_scene_denoised(const Scene& scene, ImageBuffer& image,
                               float& kernel_ms, float sigma_color) {
    // Render normalement (remplit s_renderer->d_framebuffer_)
    gpu_render_scene(scene, image, kernel_ms);

    // Le framebuffer device est dans s_renderer->d_framebuffer_
    if (!s_renderer) return;
    int w = image.width(), h = image.height();
    size_t fb_bytes = w * h * 3 * sizeof(float);

    // Buffer temporaire pour le resultat denoise
    static float* d_denoise_buf = nullptr;
    static int d_denoise_size = 0;
    if (!d_denoise_buf || d_denoise_size != w * h) {
        if (d_denoise_buf) cudaFree(d_denoise_buf);
        cudaMalloc(&d_denoise_buf, fb_bytes);
        d_denoise_size = w * h;
    }

    // Denoise sur le d_framebuffer_ du renderer (deja sur GPU)
    dim3 block(16, 16);
    dim3 grid((w+15)/16, (h+15)/16);
    denoise_bilateral_kernel<<<grid, block>>>(
        s_renderer->d_framebuffer_, d_denoise_buf, w, h, sigma_color);
    cudaDeviceSynchronize();

    // Copier le resultat denoise vers host (pinned)
    float* h_buf = nullptr;
    cudaMallocHost(&h_buf, fb_bytes);
    cudaMemcpy(h_buf, d_denoise_buf, fb_bytes, cudaMemcpyDeviceToHost);

    // Ecrire dans l'ImageBuffer
    int tp = w * h;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int px = y * w + x;
            image.set_pixel(x, y, Color(
                fminf(h_buf[px], 1.0f),
                fminf(h_buf[px+tp], 1.0f),
                fminf(h_buf[px+tp*2], 1.0f)));
        }
    cudaFreeHost(h_buf);
}

// ============================================================
// GPU-side realtime progressive rendering
// Tout reste sur le GPU : render -> accumulate -> tonemap -> transfer ARGB
// ============================================================
static float* s_rt_frame_fb = nullptr;    // framebuffer du frame courant (lineaire)
static float* s_rt_accum_fb = nullptr;    // accumulation (lineaire, somme)
static unsigned char* s_rt_argb = nullptr; // ARGB 8-bit pour affichage
static unsigned char* s_rt_argb_host = nullptr; // pinned host buffer
static int s_rt_w = 0, s_rt_h = 0;
static int s_rt_accum_count = 0;
static cudaStream_t s_rt_stream = nullptr;

void gpu_realtime_init(const Scene& scene, int width, int height) {
    s_rt_w = width; s_rt_h = height;
    size_t px = width * height;
    size_t fb_bytes = px * 3 * sizeof(float);

    if (s_rt_frame_fb) cudaFree(s_rt_frame_fb);
    if (s_rt_accum_fb) cudaFree(s_rt_accum_fb);
    if (s_rt_argb) cudaFree(s_rt_argb);
    if (s_rt_argb_host) cudaFreeHost(s_rt_argb_host);

    cudaMalloc(&s_rt_frame_fb, fb_bytes);
    cudaMalloc(&s_rt_accum_fb, fb_bytes);
    cudaMalloc(&s_rt_argb, px * 4);
    cudaMallocHost(&s_rt_argb_host, px * 4);
    cudaMemset(s_rt_accum_fb, 0, fb_bytes);
    s_rt_accum_count = 0;

    if (!s_rt_stream) cudaStreamCreate(&s_rt_stream);

    // Force scene upload
    ImageBuffer dummy(width, height);
    float km = 0;
    gpu_render_scene(scene, dummy, km);
}

// Cached per-session (pas recalcule a chaque frame)
static dim3 s_rt_block, s_rt_grid;
static int s_rt_acc_grid = 0;
static cudaEvent_t s_rt_start = nullptr, s_rt_stop = nullptr;
static bool s_rt_config_done = false;

float gpu_realtime_render_frame(const Scene& scene) {
    GCamera gcam;
    convert_camera_only(scene.camera, gcam);
    cudaMemcpyToSymbol(d_camera_bytes, &gcam, sizeof(GCamera));

    extern GpuRenderer* s_renderer;
    if (!s_renderer) return 0;

    // Config une seule fois (block size, grid, events)
    if (!s_rt_config_done) {
        s_renderer->ensure_framebuffer(s_rt_w, s_rt_h);
        int flat_block = 0, min_grid = 0;
        cudaOccupancyMaxPotentialBlockSize(&min_grid, &flat_block, render_kernel, 0, 0);
        int bx = 16, by = flat_block / bx;
        if (by < 1) by = 1; if (by > 16) by = 16;
        s_rt_block = dim3(bx, by);
        s_rt_grid = dim3((s_rt_w + bx - 1) / bx, (s_rt_h + by - 1) / by);
        int total_elems = s_rt_w * s_rt_h * 3;
        s_rt_acc_grid = (total_elems + 255) / 256;
        cudaEventCreate(&s_rt_start);
        cudaEventCreate(&s_rt_stop);
        s_rt_config_done = true;
    }

    cudaEventRecord(s_rt_start, s_rt_stream);

    // Render + accumulate, tout sur le stream (pas de sync entre les deux)
    render_kernel<<<s_rt_grid, s_rt_block, 0, s_rt_stream>>>(
        s_rt_frame_fb, s_rt_w, s_rt_h,
        s_renderer->d_bvh_nodes_, s_renderer->num_bvh_nodes_,
        s_renderer->d_spheres_, s_renderer->num_spheres_,
        s_renderer->d_planes_, s_renderer->num_planes_,
        s_renderer->d_materials_,
        s_renderer->d_soa_cx_, s_renderer->d_soa_cy_,
        s_renderer->d_soa_cz_, s_renderer->d_soa_r_, s_renderer->d_soa_mat_);

    accumulate_kernel<<<s_rt_acc_grid, 256, 0, s_rt_stream>>>(
        s_rt_frame_fb, s_rt_accum_fb, s_rt_w * s_rt_h);

    cudaEventRecord(s_rt_stop, s_rt_stream);
    // PAS de cudaStreamSynchronize ici - le tonemap synchronisera
    s_rt_accum_count++;

    float ms = 0;
    cudaEventSynchronize(s_rt_stop);
    cudaEventElapsedTime(&ms, s_rt_start, s_rt_stop);
    return ms;
}

int gpu_realtime_tonemap(unsigned char* argb_host, int width, int height) {
    dim3 block(16, 16);
    dim3 grid((width + 15) / 16, (height + 15) / 16);

    tonemap_kernel<<<grid, block, 0, s_rt_stream>>>(
        s_rt_accum_fb, s_rt_argb, width, height, s_rt_accum_count);

    // Transfer ARGB seulement (4 bytes/pixel au lieu de 12 bytes/pixel float)
    cudaMemcpyAsync(argb_host, s_rt_argb, width * height * 4,
                    cudaMemcpyDeviceToHost, s_rt_stream);
    cudaStreamSynchronize(s_rt_stream);

    return s_rt_accum_count;
}

void gpu_realtime_reset_accum() {
    if (s_rt_accum_fb)
        cudaMemset(s_rt_accum_fb, 0, s_rt_w * s_rt_h * 3 * sizeof(float));
    s_rt_accum_count = 0;
}
