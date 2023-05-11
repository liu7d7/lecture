#include <iomanip>
#include <iostream>
#include "glfw3.h"
#include "render.h"
#include "text.h"
#include "sstream"
#include "glad.h"
#include "level.h"
#include "output.h"

GLFWwindow* r_window = nullptr;

float r_width = 1152;
float r_height = 720;
float r_delta = 0.;

double r_time = 0.;
double r_prev = 0.;
double r_frametime = 0.;

bool r_paused = false;
bool r_is3d = false;

int r_mstate = GLFW_CURSOR_DISABLED;

vec3 r_pos;
vec3 r_lastpos;
vec3 r_front;
const vec3 r_worldup = vec3(0, 1, 0);
vec3 r_up = r_worldup;
vec3 r_right;
float r_pitch;
float r_yaw;
float r_lastpitch;
float r_lastyaw;
float r_mousex;
float r_mousey;
float r_lastx;
float r_lasty;

float r_speed = 1.f;

mat4 r_proj = identity<mat4>();
mat4 r_view = identity<mat4>();

t_font* r_lglight = nullptr;
t_font* r_lgregular = nullptr;
t_font* r_lgsemibold = nullptr;
t_font* r_mdlight = nullptr;
t_font* r_mdregular = nullptr;
t_font* r_mdsemibold = nullptr;
t_font* r_smlight = nullptr;
t_font* r_smregular = nullptr;
t_font* r_smsemibold = nullptr;

bool is_key_down(int key) {
  if (key < 32) return glfwGetMouseButton(r_window, key) == GLFW_PRESS;
  return glfwGetKey(r_window, key) == GLFW_PRESS;
}

void get_mouse_pos(float* x, float* y) {
  double mx, my;
  glfwGetCursorPos(r_window, &mx, &my);
  *x = (float)mx;
  *y = (float)my;
}

void r_init() {
  glfwSetInputMode(r_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glClearColor(c_blue.r, c_blue.g, c_blue.b, c_blue.a);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  r_view = identity<mat4>();

  r_lglight = t_init("res/iosevka-term-light.ttf", 256, 128);
  r_lgregular = t_init("res/iosevka-term-regular.ttf", 256, 128);
  r_lgsemibold = t_init("res/iosevka-term-semibold.ttf", 256, 128);
  r_mdlight = t_init("res/iosevka-term-light.ttf", 256, 64);
  r_mdregular = t_init("res/iosevka-term-regular.ttf", 256, 64);
  r_mdsemibold = t_init("res/iosevka-term-semibold.ttf", 256, 64);
  r_smlight = t_init("res/iosevka-term-light.ttf", 256, 24);
  r_smregular = t_init("res/iosevka-term-regular.ttf", 256, 24);
  r_smsemibold = t_init("res/iosevka-term-semibold.ttf", 256, 24);

  r_pos = r_lastpos = vec3(30, 24, 80);
  r_pitch = r_lastpitch = 0.;
  r_yaw = r_lastyaw = -90.;

  get_mouse_pos(&r_mousex, &r_mousey);
  r_lastx = r_mousex, r_lasty = r_mousey;

  l_init();
}

void update_time() {
  double t = glfwGetTime();
  if (!r_paused) r_time += r_frametime = (t - r_prev);
  r_prev = t;
}

void r_update() {
  r_lastpitch = r_pitch, r_lastyaw = r_yaw, r_lastpos = r_pos;

  if (is_key_down(GLFW_KEY_ESCAPE) && r_mstate == GLFW_CURSOR_DISABLED) {
    r_mstate = GLFW_CURSOR_NORMAL;
    glfwSetInputMode(r_window, GLFW_CURSOR, r_mstate);
  }

  if (is_key_down(GLFW_MOUSE_BUTTON_LEFT) && r_mstate != GLFW_CURSOR_DISABLED) {
    r_mstate = GLFW_CURSOR_DISABLED;
    glfwSetInputMode(r_window, GLFW_CURSOR, r_mstate);
    get_mouse_pos(&r_mousex, &r_mousey);
    r_lastx = r_mousex, r_lasty = r_mousey;
  }

  l_update();

  if (is_key_down(GLFW_KEY_LEFT) && l_allowmove)
    l_laststage();
  if (is_key_down(GLFW_KEY_RIGHT) && l_allowmove)
    l_nextstage();

  if (r_mstate == GLFW_CURSOR_NORMAL) {
    return;
  }

  get_mouse_pos(&r_mousex, &r_mousey);

  r_pitch += (r_lasty - r_mousey) * 0.1f;
  r_yaw += (r_mousex - r_lastx) * 0.1f;

  if (r_pitch > 89.f) r_pitch = 89.f;
  if (r_pitch < -89.f) r_pitch = -89.f;

  r_lastx = r_mousex, r_lasty = r_mousey;

  if (!l_allowmove) {
    r_pos = mix(r_pos, l_stageposes[l_stage].first, 0.33f);
    r_pitch = mix(r_pitch, l_stageposes[l_stage].second.y, 0.33f);
    r_yaw = mix(r_yaw, l_stageposes[l_stage].second.x, 0.33f);
    if (length(r_pos - l_stageposes[l_stage].first) < 0.1f)
      l_allowmove = true;
    return;
  }

  int forward = is_key_down(GLFW_KEY_W) - is_key_down(GLFW_KEY_S),
      right = is_key_down(GLFW_KEY_D) - is_key_down(GLFW_KEY_A),
      up = is_key_down(GLFW_KEY_SPACE) - is_key_down(GLFW_KEY_LEFT_SHIFT);

  if (forward || right || up) {
    vec3 velo = vec3(0);
    velo += r_front * (float)forward;
    velo += r_right * (float)right;
    velo += r_up * (float)up;
    velo = normalize(velo);
    r_pos += velo * r_speed;
  }
}

void update_angles() {
  float yaw = std::lerp(radians(r_lastyaw), radians(r_yaw), r_delta);
  float pitch = std::lerp(radians(r_lastpitch), radians(r_pitch), r_delta);

  r_front.x = cos(yaw) * cos(pitch);
  r_front.y = sin(pitch);
  r_front.z = sin(yaw) * cos(pitch);
  r_front = normalize(r_front);

  r_right = normalize(cross(r_front, r_worldup));
  r_up = normalize(cross(r_right, r_front));
}

void r_draw() {
  using namespace std;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  update_time();
  update_angles();

  r_3d();
  l_draw();
  t_draw(r_lgsemibold, L"&lopen&dgl&r!", {0, 0, 0}, {.outline=true, .scale=0.167f});

  r_2d();
  t_draw(r_smregular, STR("&lfps: &w", 1. / r_frametime), {0, r_height - 48.f, 0});
  t_draw(r_smregular, STR("&lxyz: &w", r_pos), {0, r_height - 72.f, 0});
  t_draw(r_smregular, STR("&lyaw: &w", r_yaw), {0, r_height - 96.f, 0});
  t_draw(r_smregular, STR("&lpitch: &w", r_pitch), {0, r_height - 120.f, 0});
}

// take 3d position and project it to 2d screen space
vec3 r_project(vec3 pos) {
  bool was3d = r_is3d;
  r_3d();

  vec4 clip = r_proj * r_view * vec4(pos, 1.f);
  vec3 ndc = vec3(clip) / clip.w;
  vec3 screen = vec3(ndc.x, ndc.y, 0.);
  screen = (screen + 1.f) / 2.f;
  screen.x *= r_width;
  screen.y *= r_height;
  screen.z = clip.w; // TODO

  if (!was3d) r_2d();
  return screen;
}

void r_3d() {
  r_is3d = true;
  r_proj = perspective(radians(45.f), r_width / r_height, 0.1f, 1000.f);

  vec3 pos = mix(r_lastpos, r_pos, r_delta);
  vec3 view_pos = pos;
  vec3 target_pos = pos + r_front;

  r_view = lookAt(view_pos, target_pos, r_up);
}

void r_2d() {
  r_is3d = false;
  r_proj = ortho(0.f, r_width, 0.f, r_height, -100.f, 100.f);
  r_view = identity<mat4>();
}