#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <example_lib.hpp>

TEST_CASE("testing the truth") { CHECK(true); }

TEST_CASE("example add function") {
  CHECK(add(1, 2) == 3);
  CHECK(add(1.5, 2.5) == 4.0);
  CHECK(add(1, 2.5) == 3.5);
}

TEST_CASE("example get_version function") { CHECK(get_version() == "0.1.0"); }
