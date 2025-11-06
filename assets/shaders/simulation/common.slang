module common;

public static const uint group_size = 256;
public static const float PI = 3.1415926;

public struct PushConstants {
    public float time;
    public float dt;
    public uint n_particles;
}

public struct BoundingBox {
    public float3 size;
    public float3 pos;
}

public namespace simulation {
    public struct SimulationParameters {
        public float3 gravity;
        public float smooth_radius;
    };

    [[vk::binding(0)]]
    public ConstantBuffer<SimulationParameters> parameters;
}

public namespace sph_model {
    public struct KernelCoefficients {
        public float spiky_pow3_scale;
        public float spiky_pow2_scale;
        public float spiky_pow3_diff_scale;
        public float spiky_pow2_diff_scale;
        public float cubic_spline_scale;
        public float grad_cubic_spline_scale;
    };

    [[vk::binding(1)]]
    public ConstantBuffer<KernelCoefficients> kernel_coeff;

    public struct Parameters {
        public float time_scale;
        public int iterations;
        public int n_particles;
        public float fixed_dt;
        public float target_density;
        public BoundingBox bounding_box;
    };

    [[vk::binding(2)]]
    public ConstantBuffer<sph_model::Parameters> parameters;

    public struct SpatialHashBuffers {
        public uint* spatial_keys;
        public uint* spatial_offsets;
        public uint* sorted_indices;
    };

    [[vk::binding(3)]]
    public ConstantBuffer<SpatialHashBuffers> spatial_hash;

    public struct ModelBuffers {
        public float3* positions;
        public float3* velocities;
        public float* densities;
        public float3* accelerations;
    };

    [[vk::binding(4)]]
    public ConstantBuffer<ModelBuffers> buffers;
}

public static const uint n_global_bindings = 5;
