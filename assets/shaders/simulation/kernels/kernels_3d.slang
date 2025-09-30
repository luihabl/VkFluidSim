module kernels_3d;

import "../common";

namespace Kernel {

    public float klinear(float dst, float radius) {
        if (dst < radius) {
            return 1 - dst / radius;
        }

        return 0;
    }

    public float poly6(float dst, float radius) {
        if (dst < radius) {
            float scale = 315 / (64 * PI * pow(abs(radius), 9));
            float v = radius * radius - dst * dst;
            return v * v * v * scale;
        }
        return 0;
    }

    public float spiky_pow3(float dst, float radius) {
        if (dst < radius) {
            float v = radius - dst;
            return v * v * v * SPHModel::kernel_coeff.spiky_pow3_scale;
        }
        return 0;
    }

    public float spiky_pow2(float dst, float radius) {
        if (dst < radius) {
            float v = radius - dst;
            return v * v * SPHModel::kernel_coeff.spiky_pow2_scale;
        }
        return 0;
    }

    public float grad_spiky_pow3(float dst, float radius) {
        if (dst <= radius) {
            float v = radius - dst;
            return -v * v * SPHModel::kernel_coeff.spiky_pow3_diff_scale;
        }
        return 0;
    }

    public float grad_spiky_pow2(float dst, float radius) {
        if (dst <= radius) {
            float v = radius - dst;
            return -v * SPHModel::kernel_coeff.spiky_pow2_diff_scale;
        }
        return 0;
    }

}
