#include "lib.hpp"

auto main() -> int
{
  library lib;

  return lib.name == "search" ? 0 : 1;
}
