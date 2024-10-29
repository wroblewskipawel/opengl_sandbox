#version 450 core

layout(location=0) in vec3 pos;

out vec3 frag_pos;

uniform mat4 proj_view;
uniform mat4 model;

void main() {
    frag_pos = pos;
    gl_Position = proj_view * model * vec4(pos, 1.0);
}