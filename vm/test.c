#include <stdio.h>
#include "vm/vm.h"

int main(int argc, char** argv)
{
  printf("convenience test file for VM\n");

  Value vs[] = { int64_to_val(1), int64_to_val(2), int64_to_val(3) };
  printf("%s", format_to_string("here's {1}, { 2 } and {   3}\n", 3, vs));
}
