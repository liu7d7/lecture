#pragma once

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include "vec4.hpp"
#include "vec3.hpp"
#include "colors.h"
#include "vec2.hpp"

using namespace glm;

struct stbtt_packedchar;

struct t_font {
  uint32_t texture;
  float height;
  int num_chars;
  float ascent;
  struct stbtt_packedchar* chars;
};

struct t_inst {
  t_font* font;
  std::wstring str;
  float x, y, outline, z, scale, r, g, b, a;

  t_inst(t_font* font, std::wstring str, float x, float y, float outline, float z, float scale, float r, float g, float b, float a) :
    font(font), str(std::move(str)), x(x), y(y), outline(outline), z(z), scale(scale), r(r), g(g), b(b), a(a) { }
};

struct t_style {
  float outline = false;
  float scale = 1.f;
  vec4 color = c_white;
  vec2 axis = vec2(1., 0.);
};

extern std::vector<float> t_verts;

extern bool t_ginit;
extern uint t_vao;
extern uint t_vbo;
extern uint t_prog;
extern int t_projloc;
extern int t_viewloc;
extern int t_texloc;

static const int t_vsize = 9;
static const int t_texsizei = 8192;
static const float t_texsize = 8192.f;
static const wchar_t t_fmt = L'&';

static vec4 with_alpha(vec4 orig, float alpha) {
  return {orig.r, orig.g, orig.b, alpha};
}

static const std::unordered_map<wchar_t, vec4(*)(vec4 color, vec4 orig)> t_fmts =
  {
    {L'r', [](vec4 color, vec4 orig) { return orig; }},
    {L'w', [](vec4 color, vec4 orig) { return with_alpha(c_white, orig.a); }},
    {L'l', [](vec4 color, vec4 orig) { return with_alpha(c_light, orig.a); }},
    {L'd', [](vec4 color, vec4 orig) { return with_alpha(c_dark, orig.a); }},
    {L'p', [](vec4 color, vec4 orig) { return with_alpha(c_pink, orig.a); }},
  };

t_font* t_init(const std::string& path, int num_chars, float height);

void t_draw(t_font* font, const std::wstring& str, vec3 pos, t_style style = t_style());