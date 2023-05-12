#include <array>
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
std::array<std::vector<l_obj*>, (int)ST_SIZE> l_entitiesbystage;
bool l_allowmove = true;

stage l_stage = ST_TITLE;
stage l_prevstage = ST_TITLE;

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
  if (glfwGetTime() > 4.) {
    vel *= 1.01f;
  }
}

void l_sphere::internal_draw() {
  vec3 p = mix(last_pos, this->pos, r_delta);
  vec3 p2d = r_project(p);
  if (p2d.z < 0) return;

  for (int i = 0; i < 6; i++) {
    verts[i * 10 + 3] = p2d.x;
    verts[i * 10 + 4] = p2d.y;
  }

  glUseProgram(l_sphereprog);
  glUniformMatrix4fv(l_projloc, 1, GL_FALSE, value_ptr(r_proj));
  glUniform1f(l_radloc, radius);
  glNamedBufferData(l_spherevbo, sizeof(float) * 6 * l_spherevsize, verts.data(), GL_DYNAMIC_DRAW);
  glBindVertexArray(l_spherevao);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

l_sphere::l_sphere(float rad, vec4 color) : radius(rad), color(color), verts(l_spheredefaults(rad * 1.25f, color)) {
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
  int size = 0;
  void (*uniforms)(l_demo*) = nullptr;

  l_demo(const std::string& vs, const std::string& fs, const std::vector<int>& attribs, std::vector<float> vertices, void (*uniforms)(l_demo*));

  ~l_demo() override;

  void internal_draw() override;
};

l_demo::l_demo(const std::string& vs, const std::string& fs, const std::vector<int>& attribs, std::vector<float> vertices, void (*uniforms)(l_demo*)) : uniforms(uniforms) {
  g_initprogram(&program, vs, fs);
  g_initvao(&vao, &vbo, attribs);
  size = vertices.size() / std::accumulate(attribs.begin(), attribs.end(), 0);
  glNamedBufferData(vbo, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
}

l_demo::~l_demo() {
  glDeleteProgram(program);
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void l_demo::internal_draw() {
  glUseProgram(program);
  uniforms(this);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, size);
}

struct l_emitter : public l_obj {
  l_obj* (*f)(vec3);
  float delay;
  float rand;
  float next_rand;
  float last_emit;
  bool first = true;
  int emitted = 0;

  l_emitter(l_obj* (*f)(vec3), float delay, float rand);

  void update() override;
};

l_emitter::l_emitter(l_obj* (*f)(vec3), float delay, float rand) : f(f), delay(delay), rand(rand), next_rand(0), last_emit((float)glfwGetTime() * 1000.f) {}

void l_emitter::update() {
  if (emitted > 2000) return;
  if ((float) glfwGetTime() * 1000.f > last_emit + delay + next_rand) {
    next_rand = (float) (rand * (float)(::rand() % 1000 - 500) / 500.f);
    last_emit = (float) glfwGetTime() * 1000.f;
    emitted += 10;
    for (int i = 0; i < 10; i++)
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
    auto* t = new l_sphere((float)(::rand() % 1000) / 1000.f * 10.f + 10.f, c_light);
    t->pos = t->last_pos = pos;
    t->vel = vec3((float)(::rand() % 1000 - 500) / 500.f, (float)(::rand() % 1000 - 500) / 500.f, (float)(::rand() % 1000 - 500) / 500.f);
    return t;
  };

  auto* emitter = new l_emitter(f, 250.f, 25.f);

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

  l_prevstage = l_stage;
  l_stage = (stage)(l_stage + 1);
  l_allowmove = false;
  std::cout << "entered " << l_stage << " at " << glfwGetTime() << std::endl;
  l_onenter();
}

void l_laststage() {
  if (l_stage == ST_TITLE) return;

  l_prevstage = l_stage;
  l_stage = (stage)(l_stage - 1);
  l_allowmove = false;
  std::cout << "entered " << l_stage << " at " << glfwGetTime() << std::endl;
  l_onenter();
}

const std::vector<std::wstring> l_introtitle {
  L"What is &lOpenGL&r?"
};

const std::vector<std::wstring> l_introinfo {
  L"- &lOpenGL&r is an &lAPI&r that allows the programmer to produce vector graphics on the &lGPU&r.",
  std::wstring(),
  L"- It serves the same purpose as &lDirect3D&r and &lVulkan&r, but is easier to use (in my opinion),",
  L"  and is cross-platform.",
  std::wstring(),
  L"- It is a huge &lstate machine&r, which means the programmer must keep track of the GPU's",
  L"  state to use it effectively."
};

const std::vector<std::wstring> l_pipelinetitle {
  L"The &lPipeline&r"
};

const std::vector<std::wstring> l_pipelineinfo {
  L"- &lOpenGL&r renders &lprimitives&r (triangles, lines, points) to the screen in stages.",
  std::wstring(),
  L"  > &lCPU&r*               - &lvertex data&r is created on the CPU.",
  L"  > &lVertex Shader&r      - vertex data is transformed to &lscreen space&r.",
  L"  > &lTesselation Shader&r - can be used to &lcreate extra detail&r and is optional.",
  L"  > &lGeometry Shader&r    - can be used to &lcreate extra geometry&r and is optional.",
  L"  > &lFragment Shader&r    - &lfragments&r (portions of a primitive) are processed into",
  L"                         color and depth information."
};

const std::vector<std::wstring> l_cputitle {
  L"The &lCPU&r"
};

const std::vector<std::wstring> l_cpuinfo {
  L"- When talking about the CPU in this context, we are referring to the &lprogram&r.",
  L"",
  L"- Though &lOpenGL&r is a graphics API, most of the code is run on the CPU and",
  L"  most of the data is produced by the CPU.",
  L"",
  L"- The CPU is responsible for creating everything that is rendered and",
  L"  anything that is used while rendering.",
  L"  > This includes &lShaders&r, &lTextures&r and &lBuffers&r (which contain",
  L"    data sent to the GPU)",
};

const std::vector<std::wstring> l_cpuexinfo {
  L"&l0&r   &pvoid&r render() {",
  L"      // creating a shader",
  L"&l1&r     &puint32_t&r vertexShader = &lglCreateShader&r(&lGL_VERTEX_SHADER&r);",
  L"      // creating a buffer",
  L"&l2&r     &puint32_t&r buffer;",
  L"&l3&r     &lglCreateBuffers&r(1, &buffer);",
  L"      // creating a texture",
  L"&l4&r     &puint32_t&r texture;",
  L"&l5&r     &lglCreateTextures&r(&lGL_TEXTURE_2D&r, 1, &texture);",
  L"      // creating a (shader) program",
  L"&l6&r     &puint32_t&l program = &lglCreateProgram&r();",
  L"      // uploading vertex data (a 1x1 square)",
  L"&l7&r     &pfloat&r vertices[] = {",
  L"&l8&r       -0.5f, -0.5f, 0.0f,",
  L"&l9&r       0.5f, -0.5f, 0.0f,",
  L"&l10&r      0.0f, 0.5f, 0.0f",
  L"&l11&r      0.0f, 0.5f, 0.0f",
  L"&l12&r      -0.5f, 0.5f, 0.0f",
  L"&l13&r      -0.5f, -0.5f, 0.0f,",
  L"&l14&r    };",
  L"&l15&r    &lglNamedBufferData&r(buffer, &psizeof&r(vertices), vertices, &lGL_STATIC_DRAW&r);",
};

const std::vector<std::wstring> l_verttitle {
  L"The &lVertex Shader&r"
};

const std::vector<std::wstring> l_vertinfo {
  L"- The Vertex Shader is the &lfirst stage&r of the pipeline.",
  L"",
  L"- It is responsible for &ltransforming&r the &lvertices&r of a primitive into &lscreen space&r.",
  L"  > Screen space, also known as &lNDC&r or &lclip space&r is used by the GPU to determine",
  L"    whether to clip, or discard, certain fragments.",
  L"",
  L"- The Vertex Shader can also be used to change the coordinates of vertices &lon the GPU&r."
};

const std::vector<std::wstring> l_vertexinfo {
  L"&l#version 460&r",
  L"",
  L"layout (location = 0) &lin&r &pvec3&r pos;",
  L"layout (location = 1) &lin&r &pvec3&r color;",
  L"",
  L"&lout&r &pvec3&r v_color;",
  L"",
  L"&luniform&r &pvec3&r translation;",
  L"&luniform&r &pmat4&r view;",
  L"&luniform&r &pmat4&r proj;",
  L"&luniform&r &pfloat&r time;",
  L"",
  L"&pvoid&r &lmain&r() {",
  L"  &pmat2&r rotation = &pmat2&r(cos(time), sin(time), -sin(time), cos(time));",
  L"  &pvec2&r rotated = rotation * pos.xz;",
  L"  &pvec3&r final = &pvec3&r(rotated.x, pos.y, rotated.y) + translation;",
  L"  &lgl_Position&r = proj * view * &pvec4&r(final, 1.);",
  L"  v_color = color;",
  L"}"
};

const std::vector<std::wstring> l_fragtitle {
  L"The &lFragment Shader&r"
};

const std::vector<std::wstring> l_fraginfo {
  L"- The Fragment Shader is the &lfinal stage&r of the pipeline.",
  L"",
  L"- It is responsible for &lprocessing&r fragments into &lcolor&r and &ldepth&r information.",
  L"",
  L"- Lighting, texturing, and post-processing happen here.",
  L"  > The vertex information and &lfragCoord&r, or position of the fragment, can be used",
  L"    to accomplish this."
};

const std::vector<std::wstring> l_fragexinfo {
  L"&l#version 460&r",
  L"",
  L"&luniform&r &pfloat&r time;",
  L"&luniform&r &pfloat&r size;",
  L"",
  L"&lin&r &pvec2&r v_uv;",
  L"&lin&r &pvec2&r v_pos;",
  L"",
  L"&lout&r &pvec4&r f_color;",
  L"",
  L"&pfloat&r transition(&pvec2&r pos) {",
  L"  &pfloat&r xFraction = &lfract&r(pos.x / size);",
  L"  &pfloat&r yFraction = &lfract&r(pos.y / size);",
  L"  &pfloat&r xDistance = &labs&r(xFraction - 0.5);",
  L"  &pfloat&r yDistance = &labs&r(yFraction - 0.5);",
  L"  &lreturn&r &pfloat&r(xDistance + yDistance + v_pos.x + v_pos.y > time * 4);",
  L"}",
  L"",
  L"&pvoid&r &lmain&r() {",
  L"  f_color = &pvec4&r(&pvec3&r(transition(&lgl_FragCoord&r.xy)), 1.0);",
  L"}",
};

const std::vector<std::wstring> l_raymarchingtitle {
  L"&lRaymarching&r"
};

const std::vector<std::wstring> l_raymarchinginfo {
  L"- &lRaymarching&r is a rendering technique where objects are defined by",
  L"  &ldistance functions&r.",
  L"",
  L"- For each fragment, a ray is cast from the camera that will define the",
  L"  color of the fragment.",
  L"  > When a ray intersects an object, the ray stops being marched along",
  L"    and the color of the object is output.",
  L"",
  L"- By using &lmod&r on the position of the ray, &lrepeating patterns&r can be created.",
};

const std::vector<std::wstring> l_geometrytitle {
  L"The &lGeometry Shader&r"
};

const std::vector<std::wstring> l_geometryinfo {
  L"- The Geometry Shader is an &loptional&r stage of the pipeline.",
  L"",
  L"- The Geometry Shader can be used to &ltransform existing geometry&r",
  L"  uploaded by the CPU into new geometry.",
  L"",
  L"- Common uses include making &lwide lines&r, displaying the &lnormals&r",
  L"  of triangle meshes, and generating &lvoxels&r from single vertices."
};

const std::vector<std::wstring> l_resourcestitle {
  L"&lResources&r"
};

const std::vector<std::wstring> l_resourcesinfo {
  L"- &lLearnOpenGL&r is a great resource for learning about OpenGL and",
  L"  is where I got started.",
  L"  > https://www.learnopengl.com/",
  L"",
  L"- &lShadertoy&r is a huge repository of cool fragment shaders",
  L"  > https://www.shadertoy.com/",
  L"",
  L"- &lThe Book of Shaders&r is a great resource for learning about",
  L"  fragment shaders.",
  L"  > https://www.thebookofshaders.com/",
  L"",
  L"- &lOpenTK&r is a super frictionless way to get into programming with",
  L"  OpenGL in C# (or F# or VB.NET).",
  L"  > https://www.opentk.net"
};

void l_onenter() {
  switch (l_stage) {
    case ST_INTRO: {
      auto* et1 = (new l_text(r_mdsemibold, l_introtitle, 72.f,{.outline=false, .scale=0.167f, .axis=vec2(1, 1.)}))->at({100, -70, -400});
      auto* et2 = (new l_text(r_mdsemibold, l_introinfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., 1.)}))->at({100, -95, -400});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_INTRO].push_back(et1);
      l_entitiesbystage[ST_INTRO].push_back(et2);
      break;
    }
    case ST_PIPELINE: {
      auto* et1 = (new l_text(r_mdsemibold, l_pipelinetitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., -1.)}))->at({-200, -70, -275});
      auto* et2 = (new l_text(r_mdsemibold, l_pipelineinfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., -1.)}))->at({-200, -95, -275});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_PIPELINE].push_back(et1);
      l_entitiesbystage[ST_PIPELINE].push_back(et2);
      break;
    }
    case ST_CPU: {
      // same as st_frag
      auto* et1 = (new l_text(r_mdsemibold, l_cputitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., -0.5)}))->at({-225, 55, -345});
      auto* et2 = (new l_text(r_mdsemibold, l_cpuinfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., -0.5)}))->at({-225, 30, -345});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_CPU].push_back(et1);
      l_entitiesbystage[ST_CPU].push_back(et2);
      break;
    }
    case ST_CPU_EX: {
      // same as st_raymarching
      auto* et1 = (new l_text(r_mdsemibold, l_cputitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.)}))->at({-360, 190, -400});
      auto* et2 = (new l_text(r_smsemibold, l_cpuexinfo, 24.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.)}))->at({-360, 175, -400});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_CPU_EX].push_back(et1);
      l_entitiesbystage[ST_CPU_EX].push_back(et2);
      break;
    }
    case ST_VERT: {
      auto* et1 = (new l_text(r_mdsemibold, l_verttitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., -1.)}))->at({-100, 160, -450});
      auto* et2 = (new l_text(r_mdsemibold, l_vertinfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., -1.)}))->at({-100, 135, -450});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_VERT].push_back(et1);
      l_entitiesbystage[ST_VERT].push_back(et2);
      break;
    }
    case ST_VERT_EX: {
      const float scale = 50.f;
      auto* et1 = (new l_text(r_mdsemibold, l_verttitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.5)}))->at({140, 55, -400});
      auto* et2 = (new l_text(r_smsemibold, l_vertexinfo, 24.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.5)}))->at({140, 45, -400});
      auto* et3 = (new l_demo("res/vertdemo.vert", "res/vertdemo.frag", { 3, 4 }, {
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
        glUniformMatrix4fv(glGetUniformLocation(self->program, "view"), 1, false, glm::value_ptr(r_view));
        glUniformMatrix4fv(glGetUniformLocation(self->program, "proj"), 1, false, glm::value_ptr(r_proj));
        glUniform1f(glGetUniformLocation(self->program, "time"), (float) glfwGetTime());
      }))->at({220, 15, -350});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_objs.push_back(et3);
      l_entitiesbystage[ST_VERT_EX].push_back(et1);
      l_entitiesbystage[ST_VERT_EX].push_back(et2);
      l_entitiesbystage[ST_VERT_EX].push_back(et3);
      break;
    }
    case ST_FRAG: {
      auto* et1 = (new l_text(r_mdsemibold, l_fragtitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., -0.5)}))->at({-245, 48, -336});
      auto* et2 = (new l_text(r_mdsemibold, l_fraginfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., -0.5)}))->at({-245, 23, -336});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_FRAG].push_back(et1);
      l_entitiesbystage[ST_FRAG].push_back(et2);
      break;
    }
    case ST_FRAG_EX: {
      auto* et1 = (new l_text(r_mdsemibold, l_fragtitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.)}))->at({-10, 45, -475});
      auto* et2 = (new l_text(r_smsemibold, l_fragexinfo, 24.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.)}))->at({-10, 30, -475});
      auto* et3 = (new l_demo("res/fragdemo.vert", "res/fragdemo.frag", { 3, 2 }, {
        -0.5f, -0.5f, 0.f, 0.f, 0.f,
        +0.5f, -0.5f, 0.f, 1.f, 0.f,
        +0.5f, +0.5f, 0.f, 1.f, 1.f,

        +0.5f, +0.5f, 0.f, 1.f, 1.f,
        -0.5f, +0.5f, 0.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.f, 0.f, 0.f,
      }, [](l_demo* self) {
        const float scale = 50.f;
        glUniform3f(glGetUniformLocation(self->program, "translation"), self->pos.x, self->pos.y, self->pos.z);
        glUniformMatrix4fv(glGetUniformLocation(self->program, "view"), 1, false, glm::value_ptr(r_view));
        glUniformMatrix4fv(glGetUniformLocation(self->program, "proj"), 1, false, glm::value_ptr(r_proj));
        float progress = fmod((float) glfwGetTime() * 1000.f - self->start, 4000.f) / 2000.f;
        if (progress > 1.f) {
          progress = 2.f - progress;
        }
        glUniform1f(glGetUniformLocation(self->program, "time"), progress);
        glUniform1f(glGetUniformLocation(self->program, "size"), 25.f);
        glUniform1f(glGetUniformLocation(self->program, "scale"), (float)scale);
      }))->at({95, 15, -475});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_objs.push_back(et3);
      l_entitiesbystage[ST_FRAG_EX].push_back(et1);
      l_entitiesbystage[ST_FRAG_EX].push_back(et2);
      l_entitiesbystage[ST_FRAG_EX].push_back(et3);
      break;
    }
    case ST_DETOUR_RAYMARCHING: {
      auto* et1 = (new l_text(r_mdsemibold, l_raymarchingtitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., -1.)}))->at({-360, 190, -400});
      auto* et2 = (new l_text(r_mdsemibold, l_raymarchinginfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., -1.)}))->at({-360, 165, -400});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_DETOUR_RAYMARCHING].push_back(et1);
      l_entitiesbystage[ST_DETOUR_RAYMARCHING].push_back(et2);
      break;
    }
    case ST_DETOUR_RAYMARCHING_EX: {
      auto* et1 = (new l_text(r_mdsemibold, l_raymarchingtitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 0.)}))->at({200, 190, -400});
      auto* et2 = (new l_demo("res/fragdemo.vert", "res/raymarching.frag", { 3, 2 }, {
        -1.25f, -0.75f, 0.f, 0.f, 0.f,
        +1.25f, -0.75f, 0.f, 0.5f, 0.f,
        +1.25f, +0.75f, 0.f, 0.5f, 0.5f,

        +1.25f, +0.75f, 0.f, 0.5f, 0.5f,
        -1.25f, +0.75f, 0.f, 0.f, 0.5f,
        -1.25f, -0.75f, 0.f, 0.f, 0.f,
      }, [](l_demo* self) {
        const float scale = 75.f;
        glUniform3f(glGetUniformLocation(self->program, "translation"), self->pos.x, self->pos.y, self->pos.z);
        glUniformMatrix4fv(glGetUniformLocation(self->program, "view"), 1, false, glm::value_ptr(r_view));
        glUniformMatrix4fv(glGetUniformLocation(self->program, "proj"), 1, false, glm::value_ptr(r_proj));
        glUniform1f(glGetUniformLocation(self->program, "time"), (float)glfwGetTime());
        glUniform1f(glGetUniformLocation(self->program, "aspect"), 2.5f / 1.5f);
        glUniform1f(glGetUniformLocation(self->program, "size"), 25.f);
        glUniform1f(glGetUniformLocation(self->program, "scale"), (float)scale);
      }))->at({293.75, 130, -400});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_DETOUR_RAYMARCHING_EX].push_back(et2);
      l_entitiesbystage[ST_DETOUR_RAYMARCHING_EX].push_back(et1);
      break;
    }
    case ST_GEOMETRY: {
      auto* et1 = (new l_text(r_mdsemibold, l_geometrytitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., -0.5)}))->at({-55, -110, -480});
      auto* et2 = (new l_text(r_mdsemibold, l_geometryinfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., -0.5)}))->at({-55, -135, -480});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_GEOMETRY].push_back(et1);
      l_entitiesbystage[ST_GEOMETRY].push_back(et2);
      break;
    }
    case ST_RESOURCES: {
      // same as st_vert
      auto* et1 = (new l_text(r_mdsemibold, l_resourcestitle, 72.f, {.outline=false, .scale=0.167f, .axis=vec2(1., 1.)}))->at({-100, 160, -400});
      auto* et2 = (new l_text(r_mdsemibold, l_resourcesinfo, 72.f, {.outline=false, .scale=0.1f, .axis=vec2(1., 1.)}))->at({-100, 135, -400});
      l_objs.push_back(et1);
      l_objs.push_back(et2);
      l_entitiesbystage[ST_RESOURCES].push_back(et1);
      l_entitiesbystage[ST_RESOURCES].push_back(et2);
      break;
    }
  }
}

const std::vector<std::pair<vec3, vec2>> l_stageposes {
  /* title */ {{28, 26, 80}, {-90, 0}},
  /* what is it */ {{14, -100, -155}, {-45, 0}},
  /* pipeline */ {{18.1, -100, -192}, {-135, 0}},
  /* cpu */ {{-43.4, 21, -173}, {-115, 0}},
  /* cpuex */ {{-297, 151, -179.5}, {-90, 0}},
  /* vert */ {{126, 131.3, -366.1}, {-135, 0}},
  /* vertex */ {{126, 22, -211}, {-65, 0}},
  /* frag */ {{-55.6, 21, -167.9}, {-115.9, 0}},
  /* fragex */ {{53, 5, -253}, {-90, 0}},
  /* detour raymarching */ {{-130.2, 151, -302.6}, {-135, 0}},
  /* raymarching ex */ {{293, 138, -200}, {-90, 0}},
  /* geometry sh */ {{110.7, -136.83, -327.463}, {-117.3, 0}},
  /* resources */ {{-223, 108.3, -156.56}, {-45, 0}},
};
