
float SmoothingKernelPoly6(float dst, float radius)
{
	if (dst < radius)
	{
		float v = radius * radius - dst * dst;
		return v * v * v * ubo.poly6_scale;
	}
	return 0;
}

float SpikyKernelPow3(float dst, float radius)
{
	if (dst < radius)
	{
		float v = radius - dst;
		return v * v * v * ubo.spiky_pow3_scale;
	}
	return 0;
}

float SpikyKernelPow2(float dst, float radius)
{
	if (dst < radius)
	{
		float v = radius - dst;
		return v * v * ubo.spiky_pow2_scale;
	}
	return 0;
}

float DerivativeSpikyPow3(float dst, float radius)
{
	if (dst <= radius)
	{
		float v = radius - dst;
		return -v * v * ubo.spiky_pow3_diff_scale;
	}
	return 0;
}

float DerivativeSpikyPow2(float dst, float radius)
{
	if (dst <= radius)
	{
		float v = radius - dst;
		return -v * ubo.spiky_pow2_diff_scale;
	}
	return 0;
}

float DensityKernel(float dst, float radius)
{
	return SpikyKernelPow2(dst, radius);
}

float NearDensityKernel(float dst, float radius)
{
	return SpikyKernelPow3(dst, radius);
}

float DensityDerivative(float dst, float radius)
{
	return DerivativeSpikyPow2(dst, radius);
}

float NearDensityDerivative(float dst, float radius)
{
	return DerivativeSpikyPow3(dst, radius);
}

float ViscosityKernel(float dst, float radius)
{
	return SmoothingKernelPoly6(dst, ubo.smoothing_radius);
}
