#version 460

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_center;
layout(location = 2) in vec4 v_color;

layout(location = 0) out vec4 f_color;

uniform float radius;
uniform float scale;

void main() {
  float dist = length(v_pos);
  if (dist > radius + radius * 0.1) {
      discard;
  }

  if (dist > radius) {
    float alpha = 1.0 - (dist - radius) / max(radius * 0.1, 0.01);
    f_color = vec4(vec3(0.), alpha * v_color.a);
    return;
  }

  if (dist > radius - radius * 0.1) {
    float alpha = 1.0 - (radius - dist) / max(radius * 0.1, 0.01);
    vec4 v = v_color;
    v.xyz *= (1 - alpha);
    f_color = v;
    return;
  }

  f_color = v_color;
  gl_FragDepth = 1.;
}