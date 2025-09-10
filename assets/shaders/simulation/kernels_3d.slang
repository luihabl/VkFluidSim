implementing "../simulation_3d.slang";

static const float PI = 3.1415926;

float LinearKernel(float dst, float radius) {
    if (dst < radius) {
        return 1 - dst / radius;
    }
    return 0;
}

float SmoothingKernelPoly6(float dst, float radius) {
    if (dst < radius) {
        float scale = 315 / (64 * PI * pow(abs(radius), 9));
        float v = radius * radius - dst * dst;
        return v * v * v * scale;
    }
    return 0;
}

float SpikyKernelPow3(float dst, float radius) {
    if (dst < radius) {
        float v = radius - dst;
        return v * v * v * ubo.spiky_pow3_scale;
    }
    return 0;
}

float SpikyKernelPow2(float dst, float radius) {
    if (dst < radius) {
        float v = radius - dst;
        return v * v * ubo.spiky_pow2_scale;
    }
    return 0;
}

float DerivativeSpikyPow3(float dst, float radius) {
    if (dst <= radius) {
        float v = radius - dst;
        return -v * v * ubo.spiky_pow3_diff_scale;
    }
    return 0;
}

float DerivativeSpikyPow2(float dst, float radius) {
    if (dst <= radius) {
        float v = radius - dst;
        return -v * ubo.spiky_pow2_diff_scale;
    }
    return 0;
}

float DensityKernel(float dst, float radius) {
    // return SmoothingKernelPoly6(dst, radius);
    return SpikyKernelPow2(dst, radius);
}

float NearDensityKernel(float dst, float radius) {
    return SpikyKernelPow3(dst, radius);
}

float DensityDerivative(float dst, float radius) {
    return DerivativeSpikyPow2(dst, radius);
}

float NearDensityDerivative(float dst, float radius) {
    return DerivativeSpikyPow3(dst, radius);
}
