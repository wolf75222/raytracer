#include "utils/denoise.h"
#include <cmath>
#include <vector>
#include <thread>

void denoise_bilateral(ImageBuffer& image, int radius, double sigma_color) {
    int w = image.width(), h = image.height();
    std::vector<Color> output(w * h);
    double sigma_spatial = (double)radius;
    const Color* src = image.data();

    int num_threads = (int)std::thread::hardware_concurrency();
    if (num_threads <= 0) num_threads = 4;

    auto worker = [&](int y_start, int y_end) {
        for (int y = y_start; y < y_end; y++) {
            for (int x = 0; x < w; x++) {
                Color center = src[y * w + x];
                Color sum(0, 0, 0);
                double weight_sum = 0;

                for (int dy = -radius; dy <= radius; dy++) {
                    for (int dx = -radius; dx <= radius; dx++) {
                        int nx = x + dx, ny = y + dy;
                        if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;

                        Color neighbor = src[ny * w + nx];

                        double spatial_dist2 = (double)(dx*dx + dy*dy);
                        double ws = std::exp(-spatial_dist2 / (2.0 * sigma_spatial * sigma_spatial));

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
    };

    // Split par bandes horizontales
    std::vector<std::thread> threads;
    int rows_per_thread = h / num_threads;
    for (int t = 0; t < num_threads; t++) {
        int y0 = t * rows_per_thread;
        int y1 = (t == num_threads - 1) ? h : y0 + rows_per_thread;
        threads.emplace_back(worker, y0, y1);
    }
    for (auto& t : threads) t.join();

    // Ecrire les resultats
    Color* dst = image.data();
    for (int i = 0; i < w * h; i++)
        dst[i] = output[i];
}
