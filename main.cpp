#include "glfw3.h"
#include "glad.h"
#include <stdexcept>
#include <vector>
#include "render.h"

std::pair<int, float> update_ticks() {
  const float tick_time = 50.f;
  static double last_frame = 0.f;
  static double prev_time = 0.f;
  static double tick_delta = 0.f;
  auto time = glfwGetTime() * 1000.;
  last_frame = (time - prev_time) / tick_time;
  prev_time = time;
  tick_delta += last_frame;
  int i = (int) tick_delta;
  tick_delta -= (float) i;
  return {i, (float) tick_delta};
}

int main() {
  if (!glfwInit()) throw std::runtime_error("failed to init GLFW!");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_SAMPLES, 4);
  r_width = glfwGetVideoMode(glfwGetPrimaryMonitor())->width;
  r_height = glfwGetVideoMode(glfwGetPrimaryMonitor())->height;
  if (!(r_window = glfwCreateWindow((int)r_width, (int)r_height, "opengl", glfwGetPrimaryMonitor(), nullptr))) throw std::runtime_error("failed to create a window!");

  glfwMakeContextCurrent(r_window);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) throw std::runtime_error("failed to load OpenGL!");

  glfwSetWindowSizeCallback(r_window, [](GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return;
    r_width = (float)width;
    r_height = (float)height;
    glViewport(0, 0, width, height);
  });

  glfwSetScrollCallback(r_window, [](GLFWwindow* window, double xoffset, double yoffset) {
    r_speed += (float)0.33f * sign((float)yoffset);
    if (r_speed < 0.1f) r_speed = 0.1f;
  });

  glEnable(GL_MULTISAMPLE);
  glViewport(0, 0, (int)r_width, (int)r_height);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  r_init();

  while (!glfwWindowShouldClose(r_window)) {
    auto tick = update_ticks();
    r_delta = tick.second;
    for (int i = 0; i < std::min(10, tick.first); i++) {
      r_update();
    }

    r_draw();

    glfwSwapBuffers(r_window);
    glfwPollEvents();
  }
  return 0;
}
