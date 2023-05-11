#pragma once

#include <ostream>
#include "vec3.hpp"
#include "vec4.hpp"
#include "fwd.hpp"
#include "detail/type_mat4x4.hpp"

template<typename O>
static O&& operator<<(O&& os, const glm::vec3& v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
  return std::forward<O>(os);
}

template<typename O>
static O&& operator<<(O&& os, const glm::vec2& v) {
  os << "(" << v.x << ", " << v.y << ")";
  return std::forward<O>(os);
}

template<typename O>
static O&& operator<<(O&& os, const glm::vec4& v) {
  os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
  return std::forward(os);
}

template<typename O>
static O&& operator<<(O&& os, const glm::mat4& m) {
  os << "(" << m[0][0] << ", " << m[0][1] << ", " << m[0][2] << ", " << m[0][3] << "), "
     << "(" << m[1][0] << ", " << m[1][1] << ", " << m[1][2] << ", " << m[1][3] << "), "
     << "(" << m[2][0] << ", " << m[2][1] << ", " << m[2][2] << ", " << m[2][3] << "), "
     << "(" << m[3][0] << ", " << m[3][1] << ", " << m[3][2] << ", " << m[3][3] << ")";
  return std::forward<O>(os);
}

#define EMPTY(...)
#define DEFER(...) __VA_ARGS__ EMPTY()
#define STR_REC_ID
#define STR_REC(x, ...) (x) __VA_OPT__(<< DEFER(STR_REC_ID)(__VA_ARGS__))
#define STR(...) (std::wostringstream() << std::fixed << std::setprecision(3) << STR_REC(__VA_ARGS__)).str()