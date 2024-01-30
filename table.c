#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void init_table(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

Object_String* table_find_string(Table* table, const char* chars, int length, uint32_t hash) {
  if (table->count == 0) return NULL;
  uint32_t index = hash % table->capacity;

  for (;;) {
    Entry* entry = &table->entries[index];
    if (entry->key == NULL) {
      // Stop if we find an empty non-tombstone entry
      if (IS_NIL(entry->value)) return NULL;
    } else if(entry->key->length == length && entry->key->hash == hash &&
        memcmp(entry->key->chars, chars, length) == 0) {
      // We found it
      return entry->key;
    }

    index = (index + 1) % table->capacity;
  }
}

static Entry* find_entry(Entry* entries, int capacity, Object_String* key) {
  uint32_t index = key->hash % capacity;
  Entry* tombstone = NULL;

  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry
        return tombstone != null ? tombstone : entry;
      } else {
        // We found a tombstone
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      // We found the key
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

// To choose the bucket for each entry, we take its hash key modulo
// the array size. That means that when the array size changes, entries may end up
// in different buckets
//
// Those new buckets may have new collisions that we need to deal with. So the
// simplest way to get every entry where it belongs is to rebuild the table from
// scratch by re-inserting every entry into the new empty array.
static void adjust_capacity(Table* table, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

    Entry* dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);

  table ->entries = entries;
  table->capacity = capacity;
}

// This function adds the given key/value pair to the given hash table. If an entry
// for that key is already present, the new value overwrites the old value. The
// function returns true if a new entry was added.
bool table_set(Table* table, Object_String* key, Value value) {

  // This is how we manage the table’s load factor. We don’t
  // grow when the capacity is completely full. Instead, we grow the array before
  // then, when the array becomes at least 75% full.
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }

  Entry* entry = find_entry(table->entries, table->capacity, key);
  bool is_new_key = entry->key == NULL;
  if (is_new_key && IS_NIL(entry->value)) table->count++;

  entry->key = key;
  entry->value = value;
  return is_new_key;
}

// It walks the bucket array of the source hash table.
// Whenever it finds a non-empty bucket, it adds the entry to the
// destination hash table using the table_set() function. (@SEE_FUNCTION :: table_set)
void table_add_all(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      table_set(to, entry->key, entry->value) ;
    }
  }
}

// You pass in a table and a key. If it finds an entry with that key, it returns
// true, otherwise it returns false. If the entry exists, the value output
// parameter points to the resulting value.
bool table_get(Table* table, Object_String* key, Value* value) {
  if (table->count == 0) return false;

  Entry* entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}

bool table_delete(Table* table, Object_String* key) {
  if (table->count == 0) return false;

  // Find the entry
  Entry* entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  // Place a tombstone in the entry
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

void free_table(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  init_table(table);
}
