#version 460 core

#extension GL_ARB_bindless_texture: require

const uint DARW_INDIRECT_BUFFER_CAPACITY = 32;

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
    vec3 norm;
    vec4 tang;
    vec2 tex;
    flat uint draw_id;
} fs_in;

out vec4 frag_color;

void main() {
    uint material_index = material_indices.draw[fs_in.draw_id];
    vec4 textureColor = texture(material_pack.materials[material_index].color_tex, fs_in.tex);
    frag_color = textureColor;
}
