#include <concepts>
#include <string>

template <typename A, typename B>
  requires(std::integral<A> || std::floating_point<A>) && (std::integral<B> || std::floating_point<B>)
auto add(A a, B b) {
  return a + b;
}

std::string get_version();
