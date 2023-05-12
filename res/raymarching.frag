#version 460

layout (location = 0) in vec2 v_uv;

layout (location = 0) out vec4 color;

uniform float time;
uniform float aspect;

const float PI = 3.14159265359;

const vec3 objs = vec3(0.93, 0.85, 0.92);
const vec3 background = vec3(0.06, 0.09, 0.13);

mat4 rot(vec3 axis, float angle) {
  axis = normalize(axis);
  float s = sin(angle);
  float c = cos(angle);
  float oc = 1. - c;
  float x = axis.x, y = axis.y, z = axis.z;

  return mat4(
  oc * x * x + c + 0, oc * x * y - z * s, oc * z * x + y * s, 0.,
  oc * x * y + z * s, oc * y * y + c + 0, oc * y * z - x * s, 0.,
  oc * z * x - y * s, oc * y * z + x * s, oc * z * z + c + 0, 0.,
  0.0000000000000000, 0.0000000000000000, 0.0000000000000000, 1.
  );
}

mat2 rot2(float angle) {
  return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}

vec3 rot(vec3 v, vec3 axis, float angle) {
  mat4 m = rot(axis, angle);
  return (m * vec4(v, 1.0)).xyz;
}

float dist_sphere(vec3 p, vec3 c, float r) {
  return length(p - c) - r;
}

float dist_cube(vec3 p, vec3 c, float r) {
  vec3 d = abs(p) - r;
  return min(max(d.x, max(d.y, d.z)), 0.) + length(max(d, 0.));
}

float scene(in vec3 p) {
  p.xyz = mod(p.xyz, 20.) - vec3(10.);
  float s0 = dist_sphere(p, vec3(0.), 1.);

  return s0;
}

vec3 norm(in vec3 p) {
  const vec3 small_step = vec3(0.001, 0., 0.);

  float gradient_x = scene(p + small_step.xyy) - scene(p - small_step.xyy);
  float gradient_y = scene(p + small_step.yxy) - scene(p - small_step.yxy);
  float gradient_z = scene(p + small_step.yyx) - scene(p - small_step.yyx);

  vec3 normal = vec3(gradient_x, gradient_y, gradient_z);

  return normalize(normal);
}

vec3 march(in vec3 ro, in vec3 rd) {
  float dist = 0.;
  const int numSteps = 256;
  const float hitDist = 0.0001;
  const float traceDist = 1920.;
  const float ambient = 0.0;

  for (int i = 0; i < numSteps; i++) {
    vec3 curPos = ro + dist * rd;
    float cdist = scene(curPos);
    if (cdist < hitDist) {
      vec3 normal = norm(curPos);
      vec3 col = objs;
      vec3 Ambient = vec3(0.81, 0.42, 0.87) * col;
      const vec3 lightPos = vec3(-1, 1., -1);
      vec3 lightDir = normalize(lightPos - curPos);
      float diff = max(dot(lightDir, normal), 0.);
      vec3 Diffuse = diff * col;
      vec3 viewDir = normalize(ro - curPos);
      vec3 reflectDir = reflect(-lightDir, normal);
      float spec = 0.;
      vec3 halfwayDir = normalize(lightDir + viewDir);
      spec = pow(max(dot(normal, halfwayDir), 0.), 32.);
      vec3 Specular = spec * col;
      return Ambient + Diffuse + Specular;
    }

    if (dist > traceDist) {
      break;
    }

    dist += cdist;
  }

  return background;
}

void main() {
  vec2 uv = v_uv;
  uv -= 0.25;
  uv *= vec2(aspect, 1.);
  vec3 cameraPos = vec3(sin(time * 0.1) * 30, sin(time * 0.1) * 30, cos(time * 0.1) * 30);
  vec3 rd = vec3((vec4(uv.x, uv.y, 1., 1.) * rot(normalize(vec3(1., 1., 1.)), time * 0.1)).xyz);
  vec3 ro = cameraPos;

  vec3 col = march(ro, rd);

  color = vec4(col, 1.);
}