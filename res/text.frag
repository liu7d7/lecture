#version 460

layout(location = 0) in vec2 v_tex;
layout(location = 1) in vec4 v_color;
layout(location = 2) in float v_outline;

out vec4 f_color;

uniform sampler2D texture;

void main() {
  vec4 c = texture(texture, v_tex);
  vec4 v = v_color;
  if (v_outline > 0.5) {
    v = mix(vec4(vec3(0.), 1.), v, c.r);
  }
  if (c.r < 0.1) discard;
  f_color = vec4(vec3(1.), c.r) * v;
}