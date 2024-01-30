#include <stdio.h>
#include <string.h>

#include "table.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJECT(type, object_type)  \
  (type*) allocate_object(sizeof(type), object_type)

// Hash function for the hash table
//  The algorithm is called “FNV-1a”
static uint32_t hash_string(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t) key[i];
    hash *= 16777619;
  }
  return hash;
}

// It allocates an object of the given size on the heap. Note that the size is
// not just the size of Obj itself. The caller passes in the number of bytes so
// that there is room for the extra payload fields needed by the specific object
// type being created.
static Object* allocate_object(size_t size, Object_Type type) {
  Object* object = (Object*) reallocate(NULL, 0, size);
  object->type = type;
  object->next = vm.objects;
  vm.objects = object;
  return object;
}

// It creates a new ObjString on the heap and then initializes its fields. It’s
// sort of like a constructor in an OOP language. As such, it first calls the “base
// class” constructor to initialize the Obj state,
static Object_String* allocate_string(char* chars, int length, uint32_t hash) {
  Object_String* string = ALLOCATE_OBJECT(Object_String, OBJECT_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  table_set(&vm.strings, string, NIL_VAL);
  return string;
}

Object_String* copy_string(const char* chars, int length) {
  uint32_t hash = hash_string(chars, length);
  Object_String* interned = table_find_string(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;
  // allocate a new array on the heap, just big enough for the string’s
  // characters and the trailing terminator
  char* heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return allocate_string(heap_chars, length, hash);
}

void print_object(Value value) {
  switch(OBJECT_TYPE(value)) {
    case OBJECT_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}

Object_String* take_string(char* chars, int length) {
  uint32_t hash = hash_string(chars, length);
  Object_String* interned = table_find_string(&vm.strings, chars, length, hash);

  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return allocate_string(chars, length, hash);
}
