module kernels_3d;

import "../common";

namespace kernel {

    public float Linear(float dst) {
        let radius = simulation::parameters.smooth_radius;
        if (dst < radius) {
            return 1 - dst / radius;
        }

        return 0;
    }

    public float Poly6(float dst) {
        let radius = simulation::parameters.smooth_radius;
        if (dst < radius) {
            float scale = 315 / (64 * PI * pow(abs(radius), 9));
            float v = radius * radius - dst * dst;
            return v * v * v * scale;
        }
        return 0;
    }

    public float SpikyPow3(float dst) {
        let radius = simulation::parameters.smooth_radius;
        if (dst < radius) {
            float v = radius - dst;
            return v * v * v * sph_model::kernel_coeff.spiky_pow3_scale;
        }
        return 0;
    }

    public float SpikyPow2(float dst) {
        let radius = simulation::parameters.smooth_radius;
        if (dst < radius) {
            float v = radius - dst;
            return v * v * sph_model::kernel_coeff.spiky_pow2_scale;
        }
        return 0;
    }

    public float GradSpikyPow3(float dst) {
        let radius = simulation::parameters.smooth_radius;
        if (dst <= radius) {
            float v = radius - dst;
            return -v * v * sph_model::kernel_coeff.spiky_pow3_diff_scale;
        }
        return 0;
    }

    public float GradSpikyPow2(float dst) {
        let radius = simulation::parameters.smooth_radius;
        if (dst <= radius) {
            float v = radius - dst;
            return -v * sph_model::kernel_coeff.spiky_pow2_diff_scale;
        }
        return 0;
    }

    public float CubicSpline(float dst) {
        let q = dst / simulation::parameters.smooth_radius;
        let sigma = sph_model::kernel_coeff.cubic_spline_scale;
        if (q <= 1.0f) {
            if (q > 0.5f) {
                return sigma * 2.0f * pow(1.0f - q, 3);
            } else {
                let q2 = q * q;
                let q3 = q2 * q;
                return sigma * (6.0f * q3 - 6.0f * q2 + 1.0f);
            }
        }

        return 0.0f;
    }

    public float GradCubicSpline(float dst) {
        let q = dst / simulation::parameters.smooth_radius;
        let sigma =
            sph_model::kernel_coeff.grad_cubic_spline_scale / simulation::parameters.smooth_radius;
        if ((dst > 1e-9) && (q <= 1.0f)) {
            if (q > 0.5f) {
                return -sigma * pow(1.0f - q, 2);
            } else {
                return sigma * q * (3.0f * q - 2.0f);
            }
        }

        return 0.0f;
    }

}
