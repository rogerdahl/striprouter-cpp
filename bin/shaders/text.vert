#version 330 core

uniform mat4 projection;

layout(location = 0) in vec3 coord3d;
layout(location = 1) in vec2 uv;

out vec2 f_uv;

void main(void)
{
  gl_Position = projection * vec4(coord3d, 1.0);
  f_uv = uv;
}
