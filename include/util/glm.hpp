#pragma once

// Project-wide GLM convenience header
// This centralizes all GLM includes in one place, making it easy to:
// 1. Include everything you need with a single #include <util/glm.hpp>
// 2. Update GLM includes project-wide from one file
// 3. Satisfy clangd's include checker with IWYU export pragmas
// 4. Keep individual files clean

// Core GLM (exported - users of util/glm.hpp get these symbols)
#include <glm/glm.hpp>  // IWYU pragma: export

// Common GLM extensions (all exported)
#include <glm/ext/matrix_clip_space.hpp>  // IWYU pragma: export (glm::ortho, glm::perspective)
#include <glm/ext/matrix_float4x4.hpp>    // IWYU pragma: export (glm::mat4)
#include <glm/ext/matrix_transform.hpp>   // IWYU pragma: export (glm::translate, glm::rotate, glm::scale)
#include <glm/ext/scalar_constants.hpp>   // IWYU pragma: export (glm::pi)
#include <glm/ext/vector_float2.hpp>      // IWYU pragma: export (glm::vec2)
#include <glm/ext/vector_float3.hpp>      // IWYU pragma: export (glm::vec3)
#include <glm/ext/vector_float4.hpp>      // IWYU pragma: export (glm::vec4)
#include <glm/gtc/matrix_transform.hpp>   // IWYU pragma: export (additional transform functions)
#include <glm/gtc/type_ptr.hpp>           // IWYU pragma: export (glm::value_ptr)
#include <glm/trigonometric.hpp>          // IWYU pragma: export (glm::radians, glm::sin, glm::cos)
#include <glm/geometric.hpp>              // IWYU pragma: export (glm::length)
#include <glm/common.hpp>                 // IWYU pragma: export (glm::clamp, glm::mix, glm::min, glm::max)

// Add more GLM headers here as needed (remember to add IWYU pragma: export):
// #include <glm/gtx/quaternion.hpp>  // IWYU pragma: export
// #include <glm/gtx/transform.hpp>   // IWYU pragma: export
