
const int nThreads = 64;

layout (binding = 0, scalar) uniform UBO {
    float dt;
    float g;
    float mass;
    float damping_factor;
    float target_density;
    float pressure_multiplier;
    float smoothing_radius;
    vec4 box;
} ubo;

struct Data {
    vec2 x;
    vec2 v;
};

layout(buffer_reference, scalar) readonly buffer DataBuffer{ 
	Data data[];
};

layout( push_constant ) uniform constants {	
    float time;
    float dt;
    uint data_buffer_size;
	DataBuffer in_buf;
	DataBuffer out_buf;
} pc;
