module kernels_3d;

import "../common";

namespace kernel {

    public float Linear(float dst, float radius) {
        if (dst < radius) {
            return 1 - dst / radius;
        }

        return 0;
    }

    public float Poly6(float dst, float radius) {
        if (dst < radius) {
            float scale = 315 / (64 * PI * pow(abs(radius), 9));
            float v = radius * radius - dst * dst;
            return v * v * v * scale;
        }
        return 0;
    }

    public float SpikyPow3(float dst, float radius) {
        if (dst < radius) {
            float v = radius - dst;
            return v * v * v * sph_model::kernel_coeff.spiky_pow3_scale;
        }
        return 0;
    }

    public float SpikyPow2(float dst, float radius) {
        if (dst < radius) {
            float v = radius - dst;
            return v * v * sph_model::kernel_coeff.spiky_pow2_scale;
        }
        return 0;
    }

    public float GradSpikyPow3(float dst, float radius) {
        if (dst <= radius) {
            float v = radius - dst;
            return -v * v * sph_model::kernel_coeff.spiky_pow3_diff_scale;
        }
        return 0;
    }

    public float GradSpikyPow2(float dst, float radius) {
        if (dst <= radius) {
            float v = radius - dst;
            return -v * sph_model::kernel_coeff.spiky_pow2_diff_scale;
        }
        return 0;
    }

}
