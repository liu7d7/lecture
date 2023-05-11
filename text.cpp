#define STB_TRUETYPE_IMPLEMENTATION

#include <stdexcept>
#include <fstream>
#include <iostream>
#include "text.h"
#include "stb_truetype.h"
#include "glad.h"
#include "render.h"
#include "gtc/type_ptr.hpp"
#include "glh.h"
#include "output.h"

typedef unsigned char byte;

std::vector<float> t_verts;

bool t_ginit;
uint t_vao;
uint t_vbo;
uint t_prog;
int t_projloc;
int t_viewloc;
int t_texloc;

t_font* t_init(const std::string& path, int num_chars, float height) {
  if (!t_ginit) {
    g_initvao(&t_vao, &t_vbo, {3, 2, 4, 1});

    g_initprogram(&t_prog, "res/text.vert", "res/text.frag");

    t_projloc = glGetUniformLocation(t_prog, "proj");
    t_viewloc = glGetUniformLocation(t_prog, "view");
    t_texloc = glGetUniformLocation(t_prog, "tex");

    t_ginit = true;
  }

  FILE* file = fopen(path.c_str(), "rb");
  fseek(file, 0, SEEK_END);
  int size = ftell(file);

  if (size == -1) {
    fclose(file);
    throw std::runtime_error("failed to read file");
  }

  fseek(file, 0, SEEK_SET);

  auto* buf = new byte[size];
  fread(buf, 1, size, file);
  fclose(file);

  auto* f = new t_font { 0, height, num_chars, 0, new stbtt_packedchar[num_chars] };

  stbtt_fontinfo font_info;
  if (!stbtt_InitFont(&font_info, buf, 0)) throw std::runtime_error("failed to init font");

  stbtt_pack_context pack_ctx;
  auto* bitmap = new byte[2048 * 2048];
  if (!stbtt_PackBegin(&pack_ctx, bitmap, 2048, 2048, 0, 1, nullptr)) throw std::runtime_error("failed to init font");
  stbtt_PackSetOversampling(&pack_ctx, 2, 2);
  stbtt_PackFontRange(&pack_ctx, buf, 0, height, 32, num_chars, f->chars);
  stbtt_PackEnd(&pack_ctx);

  int ascent;
  stbtt_GetFontVMetrics(&font_info, &ascent, nullptr, nullptr);
  f->ascent = (float) ascent * stbtt_ScaleForMappingEmToPixels(&font_info, height);

  glCreateTextures(GL_TEXTURE_2D, 1, &f->texture);
  glTextureStorage2D(f->texture, 1, GL_R8, 2048, 2048);
  glTextureSubImage2D(f->texture, 0, 0, 0, 2048, 2048, GL_RED, GL_UNSIGNED_BYTE, bitmap);
  glTextureParameteri(f->texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(f->texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(f->texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(f->texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  delete[] buf;
  delete[] bitmap;

  return f;
}

void t_draw(t_font* font, const std::wstring& str, vec3 pos, t_style style) {
  int n_verts = 6 * t_vsize * str.size();
  if (t_verts.capacity() < n_verts) t_verts.reserve(n_verts);

  int num_chars = font->num_chars;
  auto* chars = font->chars;
  int len = (int)str.size();
  vec3 d = pos;
  d.y += font->ascent * style.scale;
  vec4 orig = style.color;
  vec4 color = style.color;
  vec2 axis = normalize(style.axis);
  for (int i = 0; i < len; i++) {
    wchar_t c = str[i];
    wchar_t p = i > 0 ? str[i - 1] : 0;
    if (p == t_fmt) continue;

    if (c == t_fmt && i < len - 1) {
      wchar_t next = std::tolower(str[i + 1]);

      if (t_fmts.contains(next)) {
        auto f = t_fmts.at(next);
        color = f(color, orig);
        continue;
      }
    }

    if (c < 32 || c >= 32 + num_chars) c = ' ';

    auto pc = chars[c - 32];
    if (c != ' ')  {
      float dxs = d.x + pc.xoff * style.scale * axis.x;
      float dys = d.y - pc.yoff * style.scale;
      float dzs = d.z + pc.xoff * style.scale * axis.y;

      float dx1s = d.x + pc.xoff2 * style.scale * axis.x;
      float dy1s = d.y - pc.yoff2 * style.scale;
      float dz1s = d.z + pc.xoff2 * style.scale * axis.y;

      std::array<float, 6 * t_vsize> verts = {
        dxs, dys, dzs, (float) pc.x0 / 2048.f, (float) pc.y0 / 2048.f, color.x, color.y, color.z, color.w, style.outline,
        dxs, dy1s, dzs, (float) pc.x0 / 2048.f, (float) pc.y1 / 2048.f, color.x, color.y, color.z, color.w, style.outline,
        dx1s, dys, dz1s, (float) pc.x1 / 2048.f, (float) pc.y0 / 2048.f, color.x, color.y, color.z, color.w, style.outline,
        dx1s, dys, dz1s, (float) pc.x1 / 2048.f, (float) pc.y0 / 2048.f, color.x, color.y, color.z, color.w, style.outline,
        dxs, dy1s, dzs, (float) pc.x0 / 2048.f, (float) pc.y1 / 2048.f, color.x, color.y, color.z, color.w, style.outline,
        dx1s, dy1s, dz1s, (float) pc.x1 / 2048.f, (float) pc.y1 / 2048.f, color.x, color.y, color.z, color.w, style.outline,
      };

      t_verts.insert(t_verts.end(), verts.begin(), verts.end());
    }

    d += pc.xadvance * style.scale * 0.9f * vec3{axis.x, 0., axis.y};
  }

  glBindTexture(GL_TEXTURE_2D, font->texture);
  glUseProgram(t_prog);
  glUniformMatrix4fv(t_projloc, 1, false, value_ptr(r_proj));
  glUniformMatrix4fv(t_viewloc, 1, false, value_ptr(r_view));
  glUniform1i(t_texloc, 0);
  glNamedBufferData(t_vbo, (int64)(sizeof(float) * t_verts.size()), t_verts.data(), GL_DYNAMIC_DRAW);
  glBindVertexArray(t_vao);
  glDrawArrays(GL_TRIANGLES, 0, (int) (t_verts.size() / t_vsize));

  t_verts.clear();
}