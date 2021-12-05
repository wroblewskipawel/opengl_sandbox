#version 450 core

#extension GL_ARB_bindless_texture: require

const uint DARW_INDIRECT_BUFFER_CAPACITY = 32;
const uint MAXIMUM_LIGHT_COUNT = 8; 
const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;

struct PointLight {
    vec3 position;
    vec3 color;
};

layout(std140) uniform PointLightPack16 {
    uint count;
    PointLight light[MAXIMUM_LIGHT_COUNT];
} point_lights;

struct Material {
    vec4 color;
    vec3 emission;
    sampler2D color_tex;
    sampler2D metallic_roughness_tex;
    sampler2D normal_tex;
    sampler2D occlusion_tex;
    sampler2D emission_tex;
    float roughness;
    float metalness;
    float normal_scale;
    float occlusion_strength;
};

layout(std140) uniform pbrMaterialPack16 {
    Material materials[16];
} material_pack;

layout(std140) uniform MaterialIndices {
    uint draw[DARW_INDIRECT_BUFFER_CAPACITY];
} material_indices;

uniform samplerCube irradiance_map;
uniform samplerCube specular_map;
uniform sampler2D brdf_map;

uniform vec3 camera_pos;

in VS_OUT {
    vec3 pos;
    vec3 norm;
    vec4 tang;
    vec2 tex;
    flat uint draw_id;
} fs_in;


float attenuation(vec3 light_pos, vec3 frag_pos) {
    float distance = length(light_pos - frag_pos);
    return 1.0 / (distance * distance);
}

// f0 surface reflection at zero incidence 
vec3 fresnel_schlick(float cos_theta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 fresnel_schlick_rough(float cos_theta, vec3 f0, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float distribution_ggx(vec3 n, vec3 h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;

    float n_dot_h = max(dot(n, h), 0.0);
    float n_dot_h2 = n_dot_h * n_dot_h;

    float num = a2;
    float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float geometry_schlick_ggx(float n_dot_v, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = n_dot_v;
    float denom = n_dot_v * (1.0 - k) + k;

    return num / denom;
}

float geometry_smith(vec3 n, vec3 v, vec3 l, float roughness) {
    float n_dot_v = max(dot(n, v), 0.0);
    float n_dot_l = max(dot(n, l), 0.0);

    float ggx2 = geometry_schlick_ggx(n_dot_v, roughness);
    float ggx1 = geometry_schlick_ggx(n_dot_l, roughness);

    return ggx1 * ggx2;
}

out vec4 frag_color;

void main() {
    vec3 frag_pos = fs_in.pos;
    vec3 v = normalize(camera_pos - frag_pos);
    
    mat3 tbn = mat3(
        normalize(fs_in.tang.xyz),
        cross(fs_in.norm, fs_in.tang.xyz) * fs_in.tang.w,
        fs_in.norm
    );

    uint material_index = material_indices.draw[fs_in.draw_id];

    vec4 color_sample = texture(material_pack.materials[material_index].color_tex, fs_in.tex);
    vec4 metallic_rough_sample = texture(material_pack.materials[material_index].metallic_roughness_tex, fs_in.tex);
    vec4 normal_sample = texture(material_pack.materials[material_index].normal_tex, fs_in.tex);
    vec4 emission_sample = texture(material_pack.materials[material_index].emission_tex, fs_in.tex);    
    vec4 occlusion_sample = texture(material_pack.materials[material_index].occlusion_tex, fs_in.tex);

    vec3 n = tbn * normalize((2.0 * normal_sample.xyz - 1.0) * material_pack.materials[material_index].normal_scale);
    vec3 albedo = pow(color_sample.rgb, vec3(2.2)) * material_pack.materials[material_index].color.rgb;
    vec3 emission = emission_sample.rgb * material_pack.materials[material_index].emission;
    float occlusion = occlusion_sample.r * material_pack.materials[material_index].occlusion_strength;
    float roughness = metallic_rough_sample.g * material_pack.materials[material_index].roughness;
    float metalness = metallic_rough_sample.b * material_pack.materials[material_index].metalness;


    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metalness);
    vec3 Lo = vec3(0.0);
    for(uint i=0; i < point_lights.count; i++) {
        vec3 light_pos = point_lights.light[i].position;
        vec3 light_color = point_lights.light[i].color;

        vec3 l = normalize(light_pos - frag_pos);
        vec3 h = normalize(l + v);

        vec3 radiance = light_color * attenuation(light_pos, frag_pos);

        vec3 fresnel = fresnel_schlick(max(dot(h, v), 0.0), f0);
        float normal_dist = distribution_ggx(n, h, roughness);
        float geometry = geometry_smith(n, v, l, roughness);

        vec3 num = normal_dist * geometry * fresnel;
        float denom = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
        vec3 specular = num /denom;

        vec3 ks = fresnel;
        vec3 kd = vec3(1.0) - ks;
        kd *= 1.0 - metalness;

        float n_dot_l = max(dot(n, l), 0.0);
        Lo += (kd * albedo / PI + specular) * radiance * n_dot_l;
    }


    vec3 f = fresnel_schlick_rough(max(dot(n, v), 0.0), f0, roughness);
    vec3 ks = f;
    vec3 kd = vec3(1.0) - ks;
    kd *= 1.0 - metalness;

    vec3 irradiance = texture(irradiance_map, n).rgb;
    vec3 diffuse =  irradiance * albedo;

    vec3 r = reflect(-v, n);
    vec3 specular_color = textureLod(specular_map, r, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 env_brdf = texture(brdf_map, vec2(max(dot(n ,v), 0.0), roughness)).rg;
    vec3 specular = specular_color * (f * env_brdf.x + env_brdf.y);

    vec3 ambient = (kd * diffuse + specular) * occlusion;

    vec3 color = ambient + Lo + emission;
    color = color / (color + vec3(1.0));

    frag_color = vec4(color, 1.0);
}
