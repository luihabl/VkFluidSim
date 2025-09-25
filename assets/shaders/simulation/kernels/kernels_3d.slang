module kernels_3d;

static const float PI = 3.1415926;

struct KernelCoefficients {
    float spiky_pow3_scale;
    float spiky_pow2_scale;
    float spiky_pow3_diff_scale;
    float spiky_pow2_diff_scale;
};

[[vk::binding(1)]]
ConstantBuffer<KernelCoefficients> coeffs;

public float LinearKernel(float dst, float radius) {
    if (dst < radius) {
        return 1 - dst / radius;
    }
    return 0;
}

public float SmoothingKernelPoly6(float dst, float radius) {
    if (dst < radius) {
        float scale = 315 / (64 * PI * pow(abs(radius), 9));
        float v = radius * radius - dst * dst;
        return v * v * v * scale;
    }
    return 0;
}

public float SpikyKernelPow3(float dst, float radius) {
    if (dst < radius) {
        float v = radius - dst;
        return v * v * v * coeffs.spiky_pow3_scale;
    }
    return 0;
}

public float SpikyKernelPow2(float dst, float radius) {
    if (dst < radius) {
        float v = radius - dst;
        return v * v * coeffs.spiky_pow2_scale;
    }
    return 0;
}

public float DerivativeSpikyPow3(float dst, float radius) {
    if (dst <= radius) {
        float v = radius - dst;
        return -v * v * coeffs.spiky_pow3_diff_scale;
    }
    return 0;
}

public float DerivativeSpikyPow2(float dst, float radius) {
    if (dst <= radius) {
        float v = radius - dst;
        return -v * coeffs.spiky_pow2_diff_scale;
    }
    return 0;
}

public float DensityKernel(float dst, float radius) {
    // return SmoothingKernelPoly6(dst, radius);
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
