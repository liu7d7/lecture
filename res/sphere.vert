#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 center;
layout(location = 2) in vec4 color;

layout(location = 0) out vec3 v_pos;
layout(location = 1) out vec3 v_center;
layout(location = 2) out vec4 v_color;

uniform mat4 proj;

void main() {
  v_pos = pos;
  v_center = center;
  v_color = color;
  gl_Position = proj * vec4(pos + center, 1.0);
}