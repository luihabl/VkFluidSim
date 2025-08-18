#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require

layout (location = 0) out vec3 out_color;

struct Vertex {
    vec3 pos;
    vec4 color;
};

layout(buffer_reference, scalar) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout( push_constant ) uniform constants {	
	mat4 matrix;
	VertexBuffer vertex_buffer;
} push_constants;

void main() {
    Vertex v = push_constants.vertex_buffer.vertices[gl_VertexIndex];

    vec3 offset = vec3(0.0f, 50.0f * gl_InstanceIndex, 0.0f);

    gl_Position = push_constants.matrix * vec4(v.pos + offset, 1.0f);
    out_color = v.color.xyz;
}