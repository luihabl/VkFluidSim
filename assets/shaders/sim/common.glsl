
const int nThreads = 64;

layout(buffer_reference, scalar) readonly buffer Vec2BufferRef{ 
	vec2 data[];
};

layout (binding = 0, scalar) uniform GlobalConstants {
    float g;
    float mass;
    float damping_factor;
    float target_density;
    float pressure_multiplier;
    float smoothing_radius;    
    vec4 box;
} constants;

layout (binding = 1, scalar) uniform InputGlobalBuffers {
    Vec2BufferRef positions;
    Vec2BufferRef predicted_positions;
    Vec2BufferRef velocities;
    Vec2BufferRef densities;
} in_buffers;

layout (binding = 2, scalar) uniform OutputGlobalBuffers {
    Vec2BufferRef positions;
    Vec2BufferRef predicted_positions;
    Vec2BufferRef velocities;
    Vec2BufferRef densities;
} out_buffers;

layout( push_constant ) uniform PushConstants {	
    float time;
    float dt;
    uint n_particles;
} pc;
