
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

layout( push_constant ) uniform PushConstants {	
    float time;
    float dt;
    uint n_particles;

    Vec2BufferRef positions;
    Vec2BufferRef predicted_positions;
    Vec2BufferRef velocities;
    Vec2BufferRef densities;
} pc;
