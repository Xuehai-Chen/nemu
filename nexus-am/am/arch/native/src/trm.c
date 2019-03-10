#include <am.h>
#include <stdio.h>
#include <stdlib.h>

extern void platform_dummy();
void _trm_init() {
  platform_dummy();
}

void _putc(char ch) {
  putchar(ch);
}

void _halt(int code) {
  printf("Exit (%d)\n", code);
  exit(code);
}

_Area _heap;
