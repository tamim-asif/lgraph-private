//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "core/char_array.hpp"
#include <string>

void test0(int n) {
  Char_Array<int> test1("core_test_lgdb", "test0");
  test1.clear();
  std::string foo = "a";

  for(int i = 0; i < n; i++) {
    test1.create_id(foo.c_str(), 0x98769876);
  }
  test1.create_id("c", 2);

  assert(test1.get_field("a") == 0x98769876);
}

void test1(int n) {
  Char_Array<uint64_t> test1("core_test_lgdb", "test1");
  test1.clear();
  std::string foo = "a";

  for(int i = 0; i < n; i++) {
    test1.create_id(foo, i);
  }
  test1.create_id("c", 2);

  assert(test1.get_field("a") == n-1);
}

void test2(int n) {
  Char_Array<uint64_t> test1("core_test_lgdb", "test2");
  test1.clear();
  std::string foo = "a";

  for(int i = 0; i < n; i++) {
    foo += "a";
  }
  test1.create_id(foo.c_str(), 1);
  test1.create_id("b", 2);
}

void test3(int n) {
  Char_Array<int> test1("core_test_lgdb", "test3");
  test1.clear();
  std::string foo = "a";

  for(int i = 0; i < n; i++) {
    test1.create_id(std::to_string(i), 0x98769876);
  }
  test1.create_id("c", 2);
}

//stress test on the char array
int main(int argc, char** argv) {

  int n = 0;
  if(argc < 2) {
    //2000 is enough to trigger a couple resizings
    n = 10000;
  } else {
    n = atoi(argv[1]);
  }
  fmt::print("running test0 with {} iterations\n", n);
  test0(n);
  fmt::print("running test1 with {} iterations\n", n);
  test1(n);
  fmt::print("running test2 with {} iterations\n", n);
  test2(n);
  fmt::print("running test3 with {} iterations\n", n);
  test3(n);

  return 0;
}

