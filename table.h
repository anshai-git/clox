#ifndef lox_table_h
#define lox_table_h

#include "common.h"
#include "value.h"

typedef struct {
  Object_String* key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void init_table(Table* table);
void free_table(Table* table);
bool table_set(Table* table, Object_String* key, Value value);
void table_add_all(Table* from, Table* to);
bool table_get(Table* table, Object_String* key, Value* value);
bool table_delete(Table* table, Object_String* key);
Object_String* table_find_string(Table* table, const char* chars, int length, uint32_t hash);

#endif
