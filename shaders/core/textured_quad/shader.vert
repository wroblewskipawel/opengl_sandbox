#version 450 core

layout(location=0) in vec2 pos;
layout(location=1) in vec2 tex;

out VS_OUT {
    vec2 tex;
} vs_out;

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    vs_out.tex = tex;
}