#include "utils/denoise.h"
#include <cmath>
#include <vector>

void denoise_bilateral(ImageBuffer& image, int radius, double sigma_color) {
    int w = image.width(), h = image.height();
    std::vector<Color> output(w * h);
    double sigma_spatial = (double)radius;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Color center = image.get_pixel(x, y);
            Color sum(0, 0, 0);
            double weight_sum = 0;

            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;

                    Color neighbor = image.get_pixel(nx, ny);

                    // Distance spatiale
                    double spatial_dist = std::sqrt((double)(dx*dx + dy*dy));
                    double ws = std::exp(-spatial_dist / (2.0 * sigma_spatial * sigma_spatial));

                    // Difference de couleur
                    double dr = center.x() - neighbor.x();
                    double dg = center.y() - neighbor.y();
                    double db = center.z() - neighbor.z();
                    double color_dist = dr*dr + dg*dg + db*db;
                    double wc = std::exp(-color_dist / (2.0 * sigma_color * sigma_color));

                    double w_total = ws * wc;
                    sum += neighbor * w_total;
                    weight_sum += w_total;
                }
            }

            if (weight_sum > 0)
                output[y * w + x] = sum / weight_sum;
            else
                output[y * w + x] = center;
        }
    }

    // Ecrire les resultats
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            image.set_pixel(x, y, output[y * w + x]);
}
