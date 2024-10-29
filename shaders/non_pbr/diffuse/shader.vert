#version 460 core

#extension GL_ARB_shader_draw_parameters: require

// VERTEX ATTRIBUTES
layout(location=0) in vec3 pos;
layout(location=1) in vec3 norm;
layout(location=2) in vec4 tang;
layout(location=3) in vec2 tex;

// INSTANCE ATTRIBUTES
layout(location=4) in mat4 model;

out VS_OUT {
    vec3 norm;
    vec4 tang;
    vec2 tex;
    flat uint draw_id;
} vs_out;

uniform mat4 proj_view;

void main() {
    vs_out.norm = norm;
    vs_out.tang = tang;
    vs_out.tex= tex;
    vs_out.draw_id = gl_DrawID;
    gl_Position = proj_view * model * vec4(pos, 1.0);
}