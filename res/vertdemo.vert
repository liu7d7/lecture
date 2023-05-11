#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;

layout (location = 0) out vec3 v_color;

uniform vec3 translation;
uniform mat4 view;
uniform mat4 proj;
uniform float time;

void main() {
  mat2 rotation = mat2(cos(time), sin(time), -sin(time), cos(time));
  vec2 rotated = rotation * pos.xz;
  vec3 final = vec3(rotated.x, pos.y, rotated.y) + translation;
  gl_Position = proj * view * vec4(final, 1.);
  v_color = color;
}