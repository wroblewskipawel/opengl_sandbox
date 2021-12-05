#version 450 core

#extension GL_ARB_shader_draw_parameters: require

// VERTEX ATTRIBS
layout(location=0) in vec3 pos;
layout(location=1) in vec3 norm;
layout(location=2) in vec4 tang;
layout(location=3) in vec2 tex;

// INSTANCE ATTRIBS
layout(location=4) in mat4 model;

out VS_OUT {
    vec3 pos;
    vec3 norm;
    vec4 tang;
    vec2 tex;
    flat uint draw_id;
} vs_out;

uniform mat4 proj_view;

void main() {
    vec4 world_pos = model * vec4(pos, 1.0); 
    gl_Position = proj_view * world_pos;

    vs_out.norm = normalize(mat3(model) * norm);
    vs_out.tang = vec4(normalize(mat3(model) * tang.xyz), tang.w);
    vs_out.pos = world_pos.xyz;
    vs_out.tex = tex;
    vs_out.draw_id = gl_DrawIDARB;
}