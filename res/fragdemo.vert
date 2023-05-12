#version 460

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 v_uv;

uniform mat4 view;
uniform mat4 proj;
uniform vec3 translation;
uniform float scale;

void main() {
  gl_Position = proj * view * vec4(pos * scale + translation, 1.0);
  v_uv = uv;
}