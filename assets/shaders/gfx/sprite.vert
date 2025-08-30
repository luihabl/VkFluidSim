#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "IDL_Mac_Style.frag"

const float v_max = 6.5f;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_color;
layout(location = 0) out vec3 out_color;

layout(buffer_reference, scalar) readonly buffer DataBuffer {
    vec2 data[];
};

layout(push_constant) uniform constants {
    mat4 matrix;
    DataBuffer position;
    DataBuffer velocity;
} pc;

void main() {
    vec2 d = pc.position.data[gl_InstanceIndex];

    vec3 offset = vec3(d, 0.0f);

    gl_Position = pc.matrix * vec4(in_pos + offset, 1.0f);

    vec2 v = pc.velocity.data[gl_InstanceIndex];
    float gradient_intensity = clamp(length(v) / v_max, 0.0f, 1.0f);
    vec4 color = colormap(gradient_intensity);

    out_color = color.xyz;
}
