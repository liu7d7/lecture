#version 460

uniform float time;
uniform float size;

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 f_color;

float transition(vec2 pos) {
  pos = vec2(float(int(pos.x) / 2), float(int(pos.y) / 2)) * 2.f;
  float xFraction = fract(pos.x / size);
  float yFraction = fract(pos.y / size);
  float xDistance = abs(xFraction - 0.5);
  float yDistance = abs(yFraction - 0.5);
  return float(xDistance + yDistance + v_uv.x + v_uv.y > time * 4);
}

void main() {
  f_color = vec4(vec3(transition(gl_FragCoord.xy)), 1.0);
}