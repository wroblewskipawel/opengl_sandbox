#version 450 core

const float PI = 3.14159265359;
const uint SAMPLE_COUNT = 4096u;

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint n) {
    return vec2(float(i)/float(n), RadicalInverse_VdC(i));
}

vec3 importance_sample_ggx(vec2 xi, vec3 n, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cos_theta = sqrt((1.0-xi.y) / (1.0 + (a*a - 1.0) * xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    vec3 h;
    h.x = cos(phi) * sin_theta;
    h.y = sin(phi) * sin_theta;
    h.z = cos_theta;

    vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tang = normalize(cross(up, n));
    vec3 btang = cross(n, tang);
    vec3 sample_vec = tang * h.x + btang * h.y + n * h.z;
    return normalize(sample_vec);
}

in vec4 frag_pos;

uniform samplerCube environment_map;
uniform float roughness;

out vec4 frag_color;

void main() {
    vec3 n = normalize(frag_pos.xyz);
    vec3 r = n;
    vec3 v = r;

    float total_weight = 0.0;
    vec3 prefilter_color = vec3(0.0);
    for(uint i=0; i < SAMPLE_COUNT; i++) {
        vec2 xi = hammersley(i, SAMPLE_COUNT);
        vec3 h = importance_sample_ggx(xi, n, roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);

        float n_dot_l = max(dot(n, l), 0.0);
        if(n_dot_l > 0.0) {
            prefilter_color += texture(environment_map, l).rgb * n_dot_l;
            total_weight += n_dot_l;
        }
    }
    prefilter_color = prefilter_color / total_weight;
    frag_color = vec4(prefilter_color, 1.0);
}