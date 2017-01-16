#version 330 core

uniform mat4 projection;

layout(location = 0) in vec3 coord3d;

void main(void)
{
  gl_Position = projection * vec4(coord3d, 1.0);
}
