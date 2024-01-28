#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct Object Object;
typedef struct Object_String Object_String;

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJECT
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Object* object;
  } as;
} Value;

// Each one of these takes a C value of the appropriate type and produces a
// Value that has the correct type tag and contains the underlying value. This
// hoists statically typed values up into clox’s dynamically typed universe.
#define BOOL_VAL(value)    ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value) {VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value)  ((Value){VAL_NUMBER, {.number = value}})

// These macros go in the opposite direction. Given a Value of the right type,
// they unwrap it and return the corresponding raw C value.
// The “right type” part is important! These macros directly access the union
// fields.
#define AS_BOOL(value)     ((value).as.boolean)
#define AS_NUMBER(value)   ((value).as.number)

// These macros return true if the Value has that type.
// Any time we call one of the AS_ macros, we need to guard it behind a
// call to one of these first.
#define IS_BOOL(value)     ((value).type == VAL_BOOL)
#define IS_NIL(value)      ((value).type) == VAL_NIL
#define IS_NUMBER(value)   ((value).type == VAL_NUMBER)

// This evaluates to true if the given Value is an Obj. If so, we can use this:
#define IS_OBJECT(value)   ((value).type == VAL_OBJECT)

// It extracts the Obj pointer from the value.
#define AS_OBJECT(value)   ((value).as.object)

// This takes a bare Object pointer and wraps it in a full Value.
#define OBJECT_VAL(object) ((Value){VAL_OBJECT, {.object = (Object*)object}})

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;

bool values_equal(Value a, Value b);
void init_value_array(ValueArray* array);
void write_value_array(ValueArray* array, Value value);
void free_value_array(ValueArray* array);

void print_value(Value value);

#endif
