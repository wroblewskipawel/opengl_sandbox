#version 460 core

#extension GL_ARB_bindless_texture: require

const uint DARW_INDIRECT_BUFFER_CAPACITY = 32;

out vec4 frag_color;

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

in VS_OUT {
    flat vec3 color;
    flat uint draw_id;
} fs_in;

void main() {
    frag_color = vec4(fs_in.color, 1.0);
}