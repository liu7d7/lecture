#pragma once

#include <glm.hpp>
#include "text.h"
#include "glfw3.h"

using namespace glm;

extern GLFWwindow* r_window;

extern float r_width;
extern float r_height;
extern float r_delta;
extern float r_mousex;
extern float r_mousey;
extern float r_lastx;
extern float r_lasty;
extern float r_speed;

extern double r_time;
extern double r_prev;
extern double r_frametime;

extern bool r_paused;
extern bool r_is3d;

extern int r_mstate;

extern mat4 r_proj;
extern mat4 r_view;

extern t_font* r_lglight;
extern t_font* r_lgregular;
extern t_font* r_lgsemibold;
extern t_font* r_mdlight;
extern t_font* r_mdregular;
extern t_font* r_mdsemibold;
extern t_font* r_smlight;
extern t_font* r_smregular;
extern t_font* r_smsemibold;

void r_init();
void r_update();
void r_draw();

void r_2d();
void r_3d();

vec3 r_project(vec3 pos);