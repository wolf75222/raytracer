#ifndef RT_HYBRID_RENDERER_H
#define RT_HYBRID_RENDERER_H

#include "scene/scene.h"
#include "utils/image.h"

// Rendu hybride CPU+GPU avec work stealing par tuiles
// GPU et CPU consomment des tuiles depuis une queue partagee
void hybrid_render_scene(const Scene& scene, ImageBuffer& image,
                         float gpu_fraction,  // fraction initiale pour le GPU
                         float& cpu_time_ms, float& gpu_time_ms);

#endif // RT_HYBRID_RENDERER_H
