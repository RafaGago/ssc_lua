#include <stdio.h>
#include <ssc_lua/basic_test.h>
#include <ssc_lua/test_params.h>

int main (int argc, char const* argv[])
{
  int failed = 0;
  if (argc == 1) {
    script_path = "";
  }
  else {
    script_path = argv[1];
  }
  if (basic_tests() != 0)     { ++failed; }
  printf ("\n[SUITE ERR ] %d suite(s)\n", failed);
  return failed;
}
