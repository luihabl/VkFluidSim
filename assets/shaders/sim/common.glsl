
const int nThreads = 64;

layout(buffer_reference, scalar) readonly buffer BufferRefVec2{ 
	vec2 data[];
};

layout(buffer_reference, scalar) readonly buffer BufferRefUInt{ 
	uint data[];
};

layout (binding = 0, scalar) uniform GlobalConstants {
    float gravity;
    float mass;
    float damping_factor;
    float target_density;
    float pressure_multiplier;
    float smoothing_radius;    
    vec4 box;

    BufferRefVec2 predicted_positions;

    BufferRefUInt spatial_keys;
    BufferRefUInt spatial_offsets;
    BufferRefUInt sorted_indices;

    BufferRefVec2 sort_target_positions;
    BufferRefVec2 sort_target_pred_positions;
    BufferRefVec2 sort_target_velocities;

} ubo;

layout( push_constant ) uniform PushConstants {	
    float time;
    float dt;
    uint n_particles;

    BufferRefVec2 positions;
    BufferRefVec2 velocities;
    BufferRefVec2 densities;
} pc;
