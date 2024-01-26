#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

static void repl();
static void run_file(const char* path);
static char* read_file(const char* path);

int main(int argc, const char** argv) {
  init_vm();

  if (argc == 1) {
    repl();
  } else if(argc == 2) {
    run_file(argv[1]);
  } else {
    fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }

  free_vm();
  return 0;
}

// The difficult part is that we want to allocate a big enough string to read the whole
// file, but we don’t know how big the file is until we’ve read it.
//
// We open the file, but before reading it, we seek to the very end using fseek().
// Then we call ftell() which tells us how many bytes we are from the start of the file.
// Since we seeked (sought?) to the end, that’s the size.
// We rewind back to the beginning, allocate a string of that size,
// and read the whole file in a single batch.
static char* read_file(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not pen file <%s>\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  // Size plus one, always gotta remember to make room for the null byte.
  char* buffer = (char*) malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read <%s>\n", path);
    exit(74);
  }

  size_t bytes_read = fread(buffer ,sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file <%s> \n", path);
    exit(74);
  }
  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}

static void run_file(const char* path) {
  char* source = read_file(path);
  InterpretResult result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

static void repl() {
  char line[1024];
  for (;;) {
    printf("> ");

    if(!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    interpret(line);
  }
}
