#version 450

layout (location = 0) out vec3 out_color;

void main() {

    const vec3 pos[3] = vec3[3](
        vec3( 1.0f, 1.0f, 0.0f),
		vec3(-1.0f, 1.0f, 0.0f),
		vec3( 0.0f,-1.0f, 0.0f)
    );


    const vec3 colors[3] = vec3[3](
        vec3( 1.0f, 0.0f, 0.0f), // r
		vec3( 0.0f, 1.0f, 0.0f), // g
		vec3( 0.0f, 0.0f, 1.0f)  // b
    );

    gl_Position = vec4(pos[gl_VertexIndex], 1.0f);
    out_color = colors[gl_VertexIndex];
}