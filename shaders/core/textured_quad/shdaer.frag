#version 450 core

out vec4 frag_color;

in VS_OUT {
    vec2 tex;
} fs_in;

uniform sampler2D image;

void main() {
    frag_color = vec4(texture(image, fs_in.tex).rg, 0.0, 1.0);
};
