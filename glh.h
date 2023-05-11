#include <vector>
#include <numeric>
#include <string>
#include <fstream>
#include <iostream>
#include "glm.hpp"
#include "glad.h"

using namespace glm;

static void g_initvao(uint* vao, uint* vbo, const std::vector<int>& attribs) {
  glCreateVertexArrays(1, vao);
  glCreateBuffers(1, vbo);

  int stride = std::accumulate(attribs.begin(), attribs.end(), 0);
  glVertexArrayVertexBuffer(*vao, 0, *vbo, 0, stride * sizeof(float));

  int offset = 0;
  for (int i = 0; i < attribs.size(); i++) {
    glEnableVertexArrayAttrib(*vao, i);
    glVertexArrayAttribFormat(*vao, i, attribs[i], GL_FLOAT, GL_FALSE, offset * sizeof(float));
    glVertexArrayAttribBinding(*vao, i, 0);
    offset += attribs[i];
  }
}

static void g_initprogram(uint* prog, const std::string& vs, const std::string& fs, const std::string& gs = "") {
  std::string vss, fss, gss;
  std::ifstream vs_file(vs);
  std::ifstream fs_file(fs);
  if (!vs_file.is_open() || !fs_file.is_open()) throw std::runtime_error("failed to open shader files");
  vss = std::string(std::istreambuf_iterator<char>(vs_file), std::istreambuf_iterator<char>());
  fss = std::string(std::istreambuf_iterator<char>(fs_file), std::istreambuf_iterator<char>());
  vs_file.close();
  fs_file.close();

  if (!gs.empty()) {
    std::ifstream gs_file(gs);
    if (!gs_file.is_open()) throw std::runtime_error("failed to open shader files");
    gss = std::string(std::istreambuf_iterator<char>(gs_file), std::istreambuf_iterator<char>());
    gs_file.close();
  }

  const char* vs_src = vss.c_str();
  const char* fs_src = fss.c_str();
  const char* gs_src = gss.c_str();

  uint vsh = glCreateShader(GL_VERTEX_SHADER), fsh = glCreateShader(GL_FRAGMENT_SHADER);
  uint gsh = 0;
  if (!gs.empty()) gsh = glCreateShader(GL_GEOMETRY_SHADER);

  glShaderSource(vsh, 1, &vs_src, nullptr);
  glShaderSource(fsh, 1, &fs_src, nullptr);
  if (!gs.empty()) glShaderSource(gsh, 1, &gs_src, nullptr);

  glCompileShader(vsh);
  glCompileShader(fsh);
  if (!gs.empty()) glCompileShader(gsh);

  *prog = glCreateProgram();

  glAttachShader(*prog, vsh);
  glAttachShader(*prog, fsh);
  if (!gs.empty()) glAttachShader(*prog, gsh);

  glLinkProgram(*prog);

  glDeleteShader(vsh);
  glDeleteShader(fsh);
  if (!gs.empty()) glDeleteShader(gsh);
}

static void g_initfbo(uint* fbo, uint* color, int width, int height) {
  glCreateFramebuffers(1, fbo);
  glCreateTextures(GL_TEXTURE_2D, 1, color);
  glTextureStorage2D(*color, 1, GL_RGBA8, width, height);
  glTextureParameteri(*color, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(*color, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(*color, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(*color, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glNamedFramebufferTexture(*fbo, GL_COLOR_ATTACHMENT0, *color, 0);
}