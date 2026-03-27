// Real-time GPU raytracer with SDL3
// Pipeline GPU-side : render -> accumulate -> tonemap -> ARGB, tout sur GPU
// Seulement le buffer ARGB 8-bit est transfere au host (4 bytes/px vs 12 bytes/px)

#ifdef RT_REALTIME_ENABLED

#include "gpu/gpu_realtime.h"
#include "gpu/gpu_renderer.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <cstring>

void gpu_realtime_render(const Scene& scene, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return;
    }

    Scene& rt_scene = const_cast<Scene&>(scene);
    int orig_w = rt_scene.camera.image_width;
    int orig_spp = rt_scene.camera.samples_per_pixel;
    int orig_depth = rt_scene.camera.max_depth;
    Point3 orig_from = rt_scene.camera.lookfrom;
    Point3 orig_at = rt_scene.camera.lookat;

    rt_scene.camera.image_width = width;
    // Le spp demande par l'utilisateur est respecte
    // Pas de changement dynamique : si --spp 500, c'est 500
    rt_scene.camera.max_depth = 50;
    rt_scene.camera.initialize();
    int h = rt_scene.camera.get_image_height();

    SDL_Window* window = SDL_CreateWindow("Raytracer [GPU Real-time]", width, h, 0);
    if (!window) { fprintf(stderr, "SDL error: %s\n", SDL_GetError()); SDL_Quit(); return; }

    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* texture = SDL_CreateTexture(sdl_renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, h);

    // Init GPU-side pipeline
    gpu_realtime_init(rt_scene, width, h);

    // Host ARGB buffer (pinned, alloue par gpu_realtime_init)
    unsigned char* display = new unsigned char[width * h * 4];

    Vec3 cam_pos = rt_scene.camera.lookfrom;
    Vec3 cam_dir = unit_vector(rt_scene.camera.lookat - cam_pos);
    double cam_yaw = std::atan2(cam_dir.x(), -cam_dir.z());
    double cam_pitch = std::asin(std::clamp(cam_dir.y(), -0.99, 0.99));
    double move_speed = 2.0, mouse_sens = 0.003;
    bool mouse_captured = false, camera_moved = false;

    fprintf(stderr, "Real-time GPU-side: %dx%d | WASD + Souris | S=screenshot | ESC=quit\n", width, h);

    bool running = true;
    int frame = 0;
    int frames_since_move = 0;
    auto t_last_fps = std::chrono::high_resolution_clock::now();
    int frames_since_fps = 0;
    float current_fps = 0, last_km = 0;

    while (running) {
        camera_moved = false;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT: running = false; break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) running = false;
                if (event.key.key == SDLK_S) {
                    // Screenshot : sauve l'image actuelle en PPM
                    char fname[128];
                    snprintf(fname, sizeof(fname), "output/screenshot_%d.ppm", frame);
                    ImageBuffer screenshot(width, h);
                    // Copier depuis le display ARGB vers ImageBuffer
                    for (int y = 0; y < h; y++)
                        for (int x = 0; x < width; x++) {
                            int idx = (y * width + x) * 4;
                            double b = display[idx] / 255.0;
                            double g = display[idx+1] / 255.0;
                            double r = display[idx+2] / 255.0;
                            // Inverse gamma+ACES pour stocker lineaire dans ImageBuffer
                            screenshot.set_pixel(x, y, Color(r*r, g*g, b*b));
                        }
                    screenshot.write_ppm(fname);
                    fprintf(stderr, "Screenshot saved: %s\n", fname);
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (!mouse_captured) { SDL_SetWindowRelativeMouseMode(window, true); mouse_captured = true; }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (mouse_captured && (event.motion.xrel != 0 || event.motion.yrel != 0)) {
                    cam_yaw += event.motion.xrel * mouse_sens;
                    cam_pitch -= event.motion.yrel * mouse_sens;
                    cam_pitch = std::clamp(cam_pitch, -1.5, 1.5);
                    camera_moved = true;
                }
                break;
            }
        }

        const bool* keys = SDL_GetKeyboardState(NULL);
        Vec3 forward(std::sin(cam_yaw)*std::cos(cam_pitch), std::sin(cam_pitch), -std::cos(cam_yaw)*std::cos(cam_pitch));
        Vec3 right_vec = unit_vector(cross(forward, Vec3(0,1,0)));
        double speed = move_speed * (keys[SDL_SCANCODE_LSHIFT] ? 5.0 : 1.0);
        double dt = (current_fps > 0) ? 1.0 / current_fps : 0.016;

        Vec3 old_pos = cam_pos;
        if (keys[SDL_SCANCODE_W]) cam_pos = cam_pos + forward * (speed * dt);
        if (keys[SDL_SCANCODE_S] && !keys[SDL_SCANCODE_LCTRL]) cam_pos = cam_pos - forward * (speed * dt);
        if (keys[SDL_SCANCODE_A]) cam_pos = cam_pos - right_vec * (speed * dt);
        if (keys[SDL_SCANCODE_D]) cam_pos = cam_pos + right_vec * (speed * dt);
        if (keys[SDL_SCANCODE_SPACE]) cam_pos = cam_pos + Vec3(0,1,0) * (speed * dt);
        if (keys[SDL_SCANCODE_LCTRL]) cam_pos = cam_pos - Vec3(0,1,0) * (speed * dt);
        if ((cam_pos - old_pos).length_squared() > 1e-10) camera_moved = true;

        if (camera_moved)
            gpu_realtime_reset_accum();

        rt_scene.camera.lookfrom = cam_pos;
        rt_scene.camera.lookat = cam_pos + forward;
        rt_scene.camera.initialize();

        last_km = gpu_realtime_render_frame(rt_scene);
        frame++;
        frames_since_fps++;

        // Tonemap GPU -> ARGB host (seulement 4 bytes/px transferes)
        int total_spp = gpu_realtime_tonemap(display, width, h);

        SDL_UpdateTexture(texture, NULL, display, width * 4);
        SDL_RenderTexture(sdl_renderer, texture, NULL, NULL);
        SDL_RenderPresent(sdl_renderer);

        auto t_now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(t_now - t_last_fps).count();
        if (elapsed >= 0.5) {
            current_fps = (float)(frames_since_fps / elapsed);
            frames_since_fps = 0;
            t_last_fps = t_now;
            char title[256];
            snprintf(title, sizeof(title),
                "Raytracer [GPU] %dx%d | %.1f FPS | %.1fms | %d spp | pos(%.1f,%.1f,%.1f) | S=screenshot",
                width, h, current_fps, last_km,
                total_spp * rt_scene.camera.samples_per_pixel,
                cam_pos.x(), cam_pos.y(), cam_pos.z());
            SDL_SetWindowTitle(window, title);
        }
    }

    rt_scene.camera.image_width = orig_w;
    rt_scene.camera.samples_per_pixel = orig_spp;
    rt_scene.camera.max_depth = orig_depth;
    rt_scene.camera.lookfrom = orig_from;
    rt_scene.camera.lookat = orig_at;
    rt_scene.camera.initialize();

    delete[] display;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

#else
#include "gpu/gpu_realtime.h"
#include <cstdio>
void gpu_realtime_render(const Scene&, int, int) {
    fprintf(stderr, "Real-time requires SDL3\n");
}
#endif
