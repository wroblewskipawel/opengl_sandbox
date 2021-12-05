#version 450 core

in vec4 frag_pos;

uniform vec3 ligh_pos;
uniform float far_plane;

void main() {
    float light_distance = length(frag_pos.xyz - ligh_pos);
    light_distance = light_distance / far_plane;
    gl_FragDepth = light_distance;
}
