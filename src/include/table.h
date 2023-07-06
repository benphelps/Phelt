#ifndef phelt_table_h
#define phelt_table_h

#include "common.h"
#include "value.h"

typedef struct
{
    Value key;
    Value value;
} Entry;

typedef struct
{
    unsigned int count;
    unsigned int capacity;
    Entry*       entries;
} Table;

void       initTable(Table* table);
void       freeTable(Table* table);
bool       tableGet(Table* table, Value key, Value* value);
bool       tableSet(Table* table, Value key, Value value);
bool       tableDelete(Table* table, Value key);
void       tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
void       printTable(Table* table);

void tableRemoveWhite(Table* table);
void markTable(Table* table);

#endif
