#version 460 core

#extension GL_ARB_shader_draw_parameters : require

const uint MAX_JOIN_MATRICES = 20;

// VERTEX ATTRIBUTES
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec4 tang;
layout(location = 3) in uvec4 joints;
layout(location = 4) in vec4 weights;
layout(location = 5) in vec2 tex;

// INSTANCE ATTRIBUTES
layout(location = 6) in mat4 model;

out VS_OUT {
    vec3 norm;
    vec4 tang;
    vec2 tex;
    flat uint draw_id;
}
vs_out;

uniform mat4 proj_view;

layout(std140) uniform JointMatrices { mat4 joint[MAX_JOIN_MATRICES]; }
joint_matrices;

void main() {
    vs_out.norm = norm;
    vs_out.tang = tang;
    vs_out.tex = tex;
    vs_out.draw_id = gl_DrawID;
    mat4 skin_matrix = 
          weights.x * joint_matrices.joint[joints.x] 
        + weights.y * joint_matrices.joint[joints.y] 
        + weights.z * joint_matrices.joint[joints.z] 
        + weights.w * joint_matrices.joint[joints.w];
    gl_Position = proj_view * model * skin_matrix * vec4(pos, 1.0);
};
