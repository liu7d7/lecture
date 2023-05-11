#version 460

layout(location=0) in vec2 pos;
layout(location=1) in vec3 color;

out vec4 v_color;

uniform float time;

mat2 rotate(float angle) {
  float c = cos(angle);
  float s = sin(angle);
  return mat2(c, -s, s, c);
}

void main() {
  v_color = vec4(color, 1.0);
  //pos.x *= %f;
  pos = rotate(time) * pos;
  gl_Position = vec4(pos, 0.0, 1.0);
}