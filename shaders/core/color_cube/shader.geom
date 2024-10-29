#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices=18) out;

out vec4 frag_pos;

layout(std140) uniform CubeProjection {
    mat4 view[6];
} face_projections;

void main() {
    for(int f=0; f < 6; f++) {
        gl_Layer = f;
        for(int v=0; v < 3; v++) {
            frag_pos = gl_in[v].gl_Position;
            gl_Position = face_projections.view[f] * frag_pos;
            EmitVertex();
        }
        EndPrimitive();
    }
}