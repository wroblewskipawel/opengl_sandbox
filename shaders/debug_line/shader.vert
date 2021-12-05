#version 460 core

#extension GL_ARB_shader_draw_parameters: require

layout(location=0) in vec3 pos;
layout(location=1) in vec3 color;
layout(location=2) in mat4 model;

out VS_OUT {
    flat vec3 color;
    flat uint draw_id;
} vs_out;

uniform mat4 proj_view;

void main() {
    vs_out.color = color;
    vs_out.draw_id = gl_DrawID;
    gl_Position = proj_view * model * vec4(pos, 1.0);
}