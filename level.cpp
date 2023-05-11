#include <array>
#include <fstream>
#include <iostream>
#include <utility>
#include "level.h"
#include "render.h"
#include "glad.h"
#include "gtc/type_ptr.hpp"
#include "glh.h"

static bool l_sphereinit = false;
static uint l_spherevao = 0;
static uint l_spherevbo = 0;
static uint l_sphereprog = 0;
static int l_projloc = 0;
static int l_radloc = 0;
static const int l_spherevsize = 10;

std::vector<l_obj*> l_objs;
std::queue<int> l_holes;
std::vector<l_obj*> l_objstoadd;
std::unordered_set<stage> l_initialized { ST_TITLE };
bool l_allowmove = true;

stage l_stage = ST_TITLE;

void l_obj::draw() {
  bool was3d = r_is3d;

  if (is2d) {
    r_2d();
    internal_draw();
  } else {
    r_3d();
    internal_draw();
  }

  if (was3d) r_3d();
  else r_2d();
}

void l_obj::update() {
  last_pos = pos;
  pos += vel;
  vel *= 0.8f;
}

void l_obj::internal_draw() {

}

l_obj* l_obj::at(vec3 pos) {
  this->pos = this->last_pos = pos;
  return this;
}

std::array<float, l_spherevsize * 6> l_spheredefaults(float rad, vec4 color) {
  return
  { -rad, -rad, 0, 0, 0, 0, color.r, color.g, color.b, color.a,
      rad, -rad, 0, 0, 0, 0, color.r, color.g, color.b, color.a,
      rad, rad, 0, 0, 0, 0, color.r, color.g, color.b, color.a,
      rad, rad, 0, 0, 0, 0, color.r, color.g, color.b, color.a,
      -rad, rad, 0, 0, 0, 0, color.r, color.g, color.b, color.a,
      -rad, -rad, 0, 0, 0, 0, color.r, color.g, color.b, color.a, };
}

struct l_sphere : public l_obj {
  float radius = 15.f;
  vec4 color;

  l_sphere(float rad, vec4 color);

  void internal_draw() override;

  void update() override;
private:
  std::array<float, 6 * l_spherevsize> verts;
};

void l_sphere::update() {
  last_pos = pos;
  pos += vel;
}

void l_sphere::internal_draw() {
  vec3 p = mix(last_pos, this->pos, r_delta);
  vec3 p2d = r_project(p);
  if (p2d.z < 0) return;

  auto verts_off = verts;

  for (int i = 0; i < 6; i++) {
    verts_off[i * 10 + 3] += p2d.x;
    verts_off[i * 10 + 4] += p2d.y;
  }

  glUseProgram(l_sphereprog);
  glUniformMatrix4fv(l_projloc, 1, GL_FALSE, value_ptr(r_proj));
  glUniform1f(l_radloc, radius);
  glNamedBufferData(l_spherevbo, sizeof(float) * 6 * l_spherevsize, verts_off.data(), GL_DYNAMIC_DRAW);
  glBindVertexArray(l_spherevao);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

l_sphere::l_sphere(float rad, vec4 color) : radius(rad), color(color), verts(l_spheredefaults(rad, color)) {
  is2d = true;

  if (!l_sphereinit) {
    g_initvao(&l_spherevao, &l_spherevbo, { 3, 3, 4 });

    g_initprogram(&l_sphereprog, "res/sphere.vert", "res/sphere.frag");

    l_projloc = glGetUniformLocation(l_sphereprog, "proj");
    l_radloc = glGetUniformLocation(l_sphereprog, "radius");

    l_sphereinit = true;
  }
}

struct l_demo : public l_obj {
  uint program = 0;
  uint vao = 0;
  uint vbo = 0;
  uint size = 0;
  void (*uniforms)(l_demo*) = nullptr;

  l_demo(const std::string& vs, const std::string& fs, const std::vector<int>& attribs, std::vector<float> vertices, void (*uniforms)(l_demo*));

  void internal_draw() override;
};

l_demo::l_demo(const std::string& vs, const std::string& fs, const std::vector<int>& attribs, std::vector<float> vertices, void (*uniforms)(l_demo*)) : uniforms(uniforms) {
  g_initprogram(&program, vs, fs);
  g_initvao(&vao, &vbo, attribs);
  size = vertices.size() / std::accumulate(attribs.begin(), attribs.end(), 0);
  glNamedBufferData(vbo, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
}

void l_demo::internal_draw() {
  glUseProgram(program);
  glBindVertexArray(vao);
  uniforms(this);
  glDrawArrays(GL_TRIANGLES, 0, size);
}

struct l_emitter : public l_obj {
  l_obj* (*f)(vec3);
  float delay;
  float rand;
  float next_rand;
  float last_emit;

  l_emitter(l_obj* (*f)(vec3), float delay, float rand) : f(f), delay(delay), rand(rand), next_rand(0), last_emit((float)glfwGetTime() * 1000.f) {}

  void update() override;
};

void l_emitter::update() {
  if ((float) glfwGetTime() * 1000.f > last_emit + delay + next_rand) {
    next_rand = (float) (rand * (float)(::rand() % 1000 - 500) / 500.f);
    last_emit = (float) glfwGetTime() * 1000.f;
    l_objstoadd.push_back(f(pos));
  }
}

struct l_text : public l_obj {
  t_font* font;
  std::vector<std::wstring> text;
  float line_height;
  t_style style;

  l_text(t_font* font, std::vector<std::wstring> text, float line_height, t_style style) :
    font(font), text(std::move(text)), line_height(line_height), style(style) {}

  void internal_draw() override;
};

void l_text::internal_draw() {
  for (int i = 0; i < text.size(); i++) {
    t_draw(font, text[i], {pos.x, pos.y - (float) i * line_height * style.scale, pos.z}, style);
  }
}

void l_init() {
  auto f = +[](vec3 pos) -> l_obj* {
    auto* t = new l_sphere((float)(::rand() % 1000) / 1000.f * 10.f + 10.f, c_yellow);
    t->pos = t->last_pos = pos;
    t->vel = vec3((float)(::rand() % 1000 - 500) / 500.f, (float)(::rand() % 1000 - 500) / 500.f, (float)(::rand() % 1000 - 500) / 500.f);
    return t;
  };

  auto* emitter = new l_emitter(f, 500.f, 100.f);

  emitter->pos = emitter->last_pos = vec3(30, 24, -80);
  l_objs.push_back(emitter);
}

void l_update() {
  for (int i = 0; i < l_objs.size(); i++) {
    if (!l_objs[i]) continue;
    l_objs[i]->update();
    if (l_objs[i]->remove) {
      l_holes.push(i);
      delete l_objs[i];
      l_objs[i] = nullptr;
    }
  }

  while (!l_objstoadd.empty()) {
    if (l_holes.empty()) {
      l_objs.push_back(l_objstoadd.back());
    } else {
      l_objs[l_holes.back()] = l_objstoadd.back();
      l_holes.pop();
    }

    l_objstoadd.pop_back();
  }
}

void l_draw() {
  for (auto& obj : l_objs) {
    if (!obj) continue;
    obj->draw();
  }
}

void l_nextstage() {
  if (l_stage == ST_SIZE - 1) return;

  l_stage = (stage)(l_stage + 1);
  l_allowmove = false;
  std::cout << "entered " << l_stage << " at " << glfwGetTime() << std::endl;
  if (l_initialized.insert(l_stage).second)
    l_onenter();
}

void l_laststage() {
  if (l_stage == ST_TITLE) return;

  l_stage = (stage)(l_stage - 1);
  l_allowmove = false;
  std::cout << "entered " << l_stage << " at " << glfwGetTime() << std::endl;
  if (l_initialized.insert(l_stage).second)
    l_onenter();
}

const std::vector<std::wstring> l_introtitle {
  L"What is &yOpenGL&r?"
};

const std::vector<std::wstring> l_introinfo {
  L"- &yOpenGL&r is an &yAPI&r that allows the programmer to produce vector graphics on the &yGPU&r.",
  std::wstring(),
  L"- It serves the same purpose as &yDirect3D&r and &yVulkan&r, but is easier to use (in my opinion),",
  L"  and is cross-platform.",
  std::wstring(),
  L"- It is a &yhuge state machine&r, which means the programmer must keep track of the GPU's",
  L"  state to use it effectively."
};

const std::vector<std::wstring> l_pipelinetitle {
  L"The &yPipeline&r"
};

const std::vector<std::wstring> l_pipelineinfo {
  L"- &yOpenGL&r renders &yprimitives&r (triangles, lines, points) to the screen in stages.",
  std::wstring(),
  L"  > &yCPU&r                - vertex data is created* on the CPU.",
  L"  > &yVertex Shader&r      - vertex data is transformed to &yscreen space&r.",
  L"  > &yTesselation Shader&r - can be used to &ycreate extra detail&r and is optional.",
  L"  > &yGeometry Shader&r    - can be used to &ycreate extra geometry&r and is optional.",
  L"  > &yVertex Shader&r      - fragments (portions of a primitive) are processed into",
  L"                         color and depth information."
};

const std::vector<std::wstring> l_verttitle {
  L"The &yVertex Shader&r"
};

const std::vector<std::wstring> l_vertinfo {
  L"- The Vertex Shader is the &yfirst stage&r of the pipeline.",
  L"",
  L"- It is responsible for &ytransforming&r the &yvertices&r of a primitive into &yscreen space&r.",
  L"  > Screen space, also known as &yNDC&r or &yclip space&r is used by the GPU to determine",
  L"    whether to clip, or discard, certain fragments.",
  L"",
  L"- The Vertex Shader can also be used to change the coordinates of vertices &yon the GPU&r."
};

const std::vector<std::wstring> l_vertexinfo {
  L"&y#version 460&r",
  L"",
  L"layout (location = 0) in &dvec3&r pos;",
  L"layout (location = 1) in &dvec3&r color;",
  L"",
  L"out &dvec3&r v_color;",
  L"",
  L"&yuniform&r &dvec3&r translation;",
  L"&yuniform&r &dmat4&r view;",
  L"&yuniform&r &dmat4&r proj;",
  L"&yuniform&r &dfloat&r time;",
  L"",
  L"&dvoid&r &ymain&r() {",
  L"  &dmat2&r rotation = &dmat2&r(cos(time), sin(time), -sin(time), cos(time));",
  L"  &dvec2&r rotated = rotation * pos.xz;",
  L"  &dvec3&r final = &dvec3&r(rotated.x, pos.y, rotated.y) + translation;",
  L"  &ygl_Position&r = proj * view * &dvec4&r(final, 1.);",
  L"  v_color = color;",
  L"}"
};

void l_onenter() {
  switch (l_stage) {
    case ST_INTRO:
      l_objs.push_back((new l_text(r_mdsemibold, l_introtitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1, 1.)}))->at({100, -70, -400}));
      l_objs.push_back((new l_text(r_mdsemibold, l_introinfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., 1.)}))->at({100, -95, -400}));
      break;
    case ST_PIPELINE: {
      l_objs.push_back((new l_text(r_mdsemibold, l_pipelinetitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., -1.)}))->at({-200, -70, -275}));
      l_objs.push_back((new l_text(r_mdsemibold, l_pipelineinfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., -1.)}))->at({-200, -95, -275}));
      break;
    }
    case ST_VERT: {
      l_objs.push_back((new l_text(r_mdsemibold, l_verttitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.)}))->at({-100, 160, -400}));
      l_objs.push_back((new l_text(r_mdsemibold, l_vertinfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., 0.)}))->at({-100, 135, -400}));
      break;
    }
    case ST_VERT_EX: {
      l_objs.push_back((new l_text(r_mdsemibold, l_verttitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.5)}))->at({100, 55, -400}));
      l_objs.push_back((new l_text(r_smsemibold, l_vertexinfo, 24.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.5)}))->at({100, 45, -400}));
      const float scale = 30.f;
      l_objs.push_back((new l_demo("res/vertdemo.vert", "res/vertdemo.frag", { 3, 4 }, {
                                     -0.5f * scale, 0.000f * scale, -0.28f * scale, 1.f, 0.f, 0.f, 1.f,
                                     +0.5f * scale, 0.000f * scale, -0.28f * scale, 0.f, 1.f, 0.f, 1.f,
                                     +0.0f * scale, 0.816f * scale, +0.00f * scale, 1.f, 1.f, 1.f, 1.f,

                                     +0.0f * scale, 0.000f * scale, +0.58f * scale, 0.f, 0.f, 1.f, 1.f,
                                     +0.5f * scale, 0.000f * scale, -0.28f * scale, 0.f, 1.f, 0.f, 1.f,
                                     +0.0f * scale, 0.816f * scale, +0.00f * scale, 1.f, 1.f, 1.f, 1.f,

                                     +0.0f * scale, 0.000f * scale, +0.58f * scale, 0.f, 0.f, 1.f, 1.f,
                                     -0.5f * scale, 0.000f * scale, -0.28f * scale, 1.f, 0.f, 0.f, 1.f,
                                     +0.0f * scale, 0.816f * scale, +0.00f * scale, 1.f, 1.f, 1.f, 1.f,

                                     -0.5f * scale, 0.000f * scale, -0.28f * scale, 1.f, 0.f, 0.f, 1.f,
                                     +0.5f * scale, 0.000f * scale, -0.28f * scale, 0.f, 1.f, 0.f, 1.f,
                                     +0.0f * scale, 0.000f * scale, +0.58f * scale, 0.f, 0.f, 1.f, 1.f,
                                   }, [](l_demo* self) {
                                     glUniform3f(glGetUniformLocation(self->program, "translation"), self->pos.x, self->pos.y, self->pos.z);
                                     glUniformMatrix4fv(glGetUniformLocation(self->program, "view"), 1, GL_FALSE, glm::value_ptr(r_view));
                                     glUniformMatrix4fv(glGetUniformLocation(self->program, "proj"), 1, GL_FALSE, glm::value_ptr(r_proj));
                                     glUniform1f(glGetUniformLocation(self->program, "time"), (float) glfwGetTime());
                                   }))->at({180, 15, -350}));
    }
  }
}

const std::vector<std::pair<vec3, vec2>> l_stageposes {
  {{30, 24, 80}, {-90, 0}},
  {{14, -100, -155}, {-45, 0}},
  {{16, -100, -190}, {-135, 0}},
  {{4, 131, -179}, {-90, 0}},
  {{86, 22, -211}, {-65, 0}},
};