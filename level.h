#pragma once

#include <vector>
#include <vec3.hpp>
#include <queue>
#include <unordered_set>
#include "vec2.hpp"
#include "glfw3.h"

using namespace glm;

struct l_obj {
  vec3 pos = vec3(0.);
  vec3 last_pos = vec3(0.);
  vec3 vel = vec3(0.);
  bool is2d = false;
  bool remove = false;
  float start = (float)glfwGetTime() * 1000.f;

  virtual ~l_obj() = default;

  virtual void update();

  void draw();

  virtual void internal_draw();

  l_obj* at(vec3 pos);
};

enum stage {
  ST_TITLE,
  ST_INTRO,
  ST_PIPELINE,
  ST_VERT,
  ST_VERT_EX,
  ST_FRAG,
  ST_FRAG_EX,
  ST_DETOUR_RAYMARCHING,
  ST_DETOUR_SHADERTOY,
  ST_GEOMETRY,
  ST_GEOMETRY_EX,
  ST_TESSELATION,
  ST_RESOURCES,
  ST_SIZE
};

extern std::vector<l_obj*> l_objs;
extern std::queue<int> l_holes;
extern stage l_stage;
extern std::unordered_set<stage> l_initialized;
extern bool l_allowmove;
extern const std::vector<std::pair<vec3, vec2>> l_stageposes;

void l_init();
void l_update();
void l_draw();

void l_nextstage();
void l_laststage();
void l_onenter();