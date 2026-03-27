#ifndef RT_UTILS_DENOISE_H
#define RT_UTILS_DENOISE_H

#include "utils/image.h"

// Simple bilateral denoise : lisse le bruit tout en preservant les contours
// sigma_spatial : rayon du filtre (en pixels)
// sigma_color : sensibilite aux differences de couleur
void denoise_bilateral(ImageBuffer& image, int radius = 3, double sigma_color = 0.1);

#endif
