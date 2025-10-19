#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <util/types.hpp>
#include <util/glm.hpp>

TEST_CASE("testing glm::vec3 addition") {
  glm::vec3 a(1.0f, 2.0f, 3.0f);
  glm::vec3 b(4.0f, 5.0f, 6.0f);
  glm::vec3 c = a + b;
  CHECK(c.x == doctest::Approx(5.0f));
  CHECK(c.y == doctest::Approx(7.0f));
  CHECK(c.z == doctest::Approx(9.0f));
}
