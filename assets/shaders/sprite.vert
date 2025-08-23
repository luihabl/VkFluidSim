#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec4 in_color;
layout (location = 0) out vec3 out_color;

struct Data {
    float x;
    float v;
};

layout(buffer_reference, scalar) readonly buffer DataBuffer{ 
	Data data[];
};


layout( push_constant ) uniform constants {	
	mat4 matrix;
	DataBuffer pos_buffer;
} push_constants;

void main() {
    Data d = push_constants.pos_buffer.data[gl_InstanceIndex];

    vec3 offset = vec3(50.0f * d.x, 15.0f * gl_InstanceIndex - 300.0f, 0.0f);

    gl_Position = push_constants.matrix * vec4(in_pos + offset, 1.0f);
    out_color = in_color.xyz;
}