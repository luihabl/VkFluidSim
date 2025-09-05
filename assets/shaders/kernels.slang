module kernels;

public float SmoothingKernelPoly6(float dst, float radius) {
    if (dst < radius) {
        float v = radius * radius - dst * dst;
        return v * v * v * ubo.poly6_scale;
    }
    return 0;
}

public float SpikyKernelPow3(float dst, float radius) {
    if (dst < radius) {
        float v = radius - dst;
        return v * v * v * ubo.spiky_pow3_scale;
    }
    return 0;
}

public float SpikyKernelPow2(float dst, float radius) {
    if (dst < radius) {
        float v = radius - dst;
        return v * v * ubo.spiky_pow2_scale;
    }
    return 0;
}

public float DerivativeSpikyPow3(float dst, float radius) {
    if (dst <= radius) {
        float v = radius - dst;
        return -v * v * ubo.spiky_pow3_diff_scale;
    }
    return 0;
}

public float DerivativeSpikyPow2(float dst, float radius) {
    if (dst <= radius) {
        float v = radius - dst;
        return -v * ubo.spiky_pow2_diff_scale;
    }
    return 0;
}

public float DensityKernel(float dst, float radius) {
    return SpikyKernelPow2(dst, radius);
}

public float NearDensityKernel(float dst, float radius) {
    return SpikyKernelPow3(dst, radius);
}

public float DensityDerivative(float dst, float radius) {
    return DerivativeSpikyPow2(dst, radius);
}

public float NearDensityDerivative(float dst, float radius) {
    return DerivativeSpikyPow3(dst, radius);
}

public float ViscosityKernel(float dst, float radius) {
    return SmoothingKernelPoly6(dst, ubo.smoothing_radius);
}
