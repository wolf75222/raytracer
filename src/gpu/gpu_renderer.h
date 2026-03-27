#ifndef RT_GPU_RENDERER_H
#define RT_GPU_RENDERER_H

// GPU renderer interface - includable by MSVC (no CUDA qualifiers in this header)
// The actual GpuRenderer class internals are in gpu_renderer.cu

#include "scene/scene.h"
#include "utils/image.h"

// Forward declarations for GPU types (defined in gpu_types.cuh, only visible to nvcc)
struct GSphere;
struct GPlane;
struct GMaterial;
struct GBVHNode;
struct GCamera;

// Host-side GPU renderer interface
class GpuRenderer {
public:
    GpuRenderer();
    ~GpuRenderer();

    // Upload scene data to GPU (call once)
    void upload_scene(const GSphere* spheres, int num_spheres,
                      const GPlane* planes, int num_planes,
                      const GMaterial* materials, int num_materials,
                      const GBVHNode* bvh_nodes, int num_bvh_nodes);

    // Render to host buffer (RGB float, 3 floats per pixel)
    void render(const GCamera& camera, float* h_output, int width, int height);

    // Get last render time in ms
    float last_render_ms() const { return last_ms_; }

    // Public accessors for realtime GPU rendering
    friend float gpu_realtime_render_frame(const Scene&);
    friend void gpu_render_scene_denoised(const Scene&, ImageBuffer&, float&, float);

private:
    GSphere* d_spheres_ = nullptr;
    // SoA spheres
    float* d_soa_cx_ = nullptr;
    float* d_soa_cy_ = nullptr;
    float* d_soa_cz_ = nullptr;
    float* d_soa_r_ = nullptr;
    int* d_soa_mat_ = nullptr;
    GPlane* d_planes_ = nullptr;
    GMaterial* d_materials_ = nullptr;
    GBVHNode* d_bvh_nodes_ = nullptr;
    float* d_framebuffer_ = nullptr;

    int num_spheres_ = 0;
    int num_planes_ = 0;
    int num_materials_ = 0;
    int num_bvh_nodes_ = 0;
    int fb_width_ = 0;
    int fb_height_ = 0;

    float last_ms_ = 0.0f;

public:
    void ensure_framebuffer(int width, int height);
private:
    void free_scene();
    void free_framebuffer();
};

// Bridge function: converts CPU Scene -> GPU data + renders
// Defined in gpu_renderer.cu (compiled by nvcc)
void gpu_render_scene(const Scene& scene, ImageBuffer& image,
                      float& kernel_ms);

// GPU render + bilateral denoise sur GPU
void gpu_render_scene_denoised(const Scene& scene, ImageBuffer& image,
                               float& kernel_ms, float sigma_color = 0.1f);

// GPU-side progressive rendering (no host transfer per frame)
// Initialise les buffers GPU pour l'accumulation
void gpu_realtime_init(const Scene& scene, int width, int height);
// Rend 1 frame et accumule sur GPU, retourne le kernel time
float gpu_realtime_render_frame(const Scene& scene);
// Tonemap GPU -> host ARGB buffer, retourne total samples
int gpu_realtime_tonemap(unsigned char* argb_host, int width, int height);
// Reset l'accumulation (quand camera bouge)
void gpu_realtime_reset_accum();

#endif // RT_GPU_RENDERER_H
