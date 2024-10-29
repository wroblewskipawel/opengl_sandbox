#version 450 core

const vec2 INV_ATAN = vec2(0.1591, 0.3183);

in vec4 frag_pos;

out vec4 frag_color;

uniform sampler2D equirectangular_map;


vec2 sample_spherical(vec3 v) {
    vec2 uv = vec2(atan(v.y, v.x), asin(v.z));
    uv *= INV_ATAN;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

void main() {
    vec2 uv = sample_spherical(normalize(frag_pos.xyz));
    frag_color  = vec4(texture(equirectangular_map, uv).rgb, 1.0);
}