#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"


#define OBJECT_TYPE(value)  (AS_OBJECT(value)->type)
#define IS_STRING(value)    is_object(value, OBJECT_STRING)

// These two macros take a Value that is expected to contain a pointer to a valid
// ObjString on the heap. The first one returns the ObjString* pointer. The
// second one steps through that to return the character array itself
#define AS_STRING(value)    ((Object_String*) AS_OBJECT(value))
#define AS_CSTRING(value)   (((Object_String*) AS_OBJECT(value))->chars)

typedef enum {
  OBJECT_STRING,
} Object_Type;

struct Object {
  Object_Type type;
  struct Object* next;
};

// Within a structure object, the non-bit-field members and the units in which
// bit-fields reside have addresses that increase in the order in which they
// are declared. A pointer to a structure object, suitably converted, points to
// its initial member (or if that member is a bit-field, then to the unit in
// which it resides), and vice versa. There may be unnamed padding within a
// structure object, but not at its beginning.
//
// Given an ObjString*, you can safely cast it to Obj* and then access the
// type field from it. Every ObjString “is” an Obj in the OOP sense of “is”.
struct Object_String {
  Object object;
  int length;
  char* chars;
  uint32_t hash;
};

Object_String* copy_string(const char* chars, int length);
void print_object(Value value);
Object_String* take_string(char* chars, int length);

static inline bool is_object(Value value, Object_Type type) {
  return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#endif
