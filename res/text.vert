#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec4 color;

layout(location = 0) out vec2 v_tex;
layout(location = 1) out vec4 v_color;

uniform mat4 proj;
uniform mat4 view;

void main() {
  gl_Position = proj * view * vec4(pos, 1.0);
  v_tex = tex;
  v_color = color;
}