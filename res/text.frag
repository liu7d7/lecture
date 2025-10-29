#version 460

layout(location = 0) in vec2 v_tex;
layout(location = 1) in vec4 v_color;

out vec4 f_color;

vec4 hack(sampler2D samp) {
  return texture(samp, v_tex);
}

uniform sampler2D texture;

void main() {
  vec4 c = hack(texture);
  vec4 v = v_color;
  if (c.r < 0.01) {
    discard;
  }
  f_color = vec4(vec3(1.), c.r) * v;
}
