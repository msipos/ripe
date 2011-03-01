#include <stdio.h>
#include "vm/vm.h"

int main(int argc, char** argv)
{
  printf("convenience test file for VM\n");
  FormatParse fp;
  format_parse("this is a format string number {1} and { width = 8 } {3}", &fp);
  for (int i = 0; i < fp.size; i++){
    printf("%d (%d): '%s'\n", i, fp.elements[i].type, fp.elements[i].str);
  }
}
