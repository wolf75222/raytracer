#ifndef RT_GPU_REALTIME_H
#define RT_GPU_REALTIME_H

#include "scene/scene.h"
#include "utils/image.h"

// Lance le rendu GPU temps reel dans une fenetre Win32
// Boucle infinie : 1 spp par frame, affiche FPS
// Appuyer sur ESC pour quitter
void gpu_realtime_render(const Scene& scene, int width, int height);

#endif
