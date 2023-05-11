#version 460

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_center;
layout(location = 2) in vec4 v_color;

layout(location = 0) out vec4 f_color;

uniform float radius;

void main() {
  float dist = length(v_pos);
  if (dist > radius + radius * 0.05) {
      discard;
  }
  if (dist > radius) {
    float alpha = 1.0 - (dist - radius) / (radius * 0.05);
    f_color = vec4(v_color.rgb, alpha * v_color.a);
    return;
  }
  f_color = v_color;
  gl_FragDepth = 1.;
}