#version 330 core

uniform sampler2D s;
in vec2 f_uv;
out vec4 color;

void main(void) {
    color = texelFetch(s, ivec2(f_uv), 0).rgba;
    color.a = 1.0;
}
