#include "table.h"
#include "common.h"
#include "memory.h"
#include "object.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.85

void initTable(Table* table)
{
    table->count    = 0;
    table->capacity = 0;
    table->entries  = NULL;
}

void freeTable(Table* table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity, Value key)
{
    Entry* tombstone = NULL;

    uint32_t index = hashValue(key) & (capacity - 1);
    for (;;) {
        Entry* entry = &entries[index];
        if (IS_EMPTY(entry->key)) {
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone.
                if (tombstone == NULL)
                    tombstone = entry;
            }
        } else if (valuesEqual(key, entry->key)) {
            // We found the key.
            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

bool tableGet(Table* table, Value key, Value* value)
{
    if (table->count == 0)
        return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (IS_EMPTY(entry->key))
        return false;

    if (value != NULL)
        *value = entry->value;
    return true;
}

static void adjustCapacity(Table* table, unsigned int capacity)
{
    Entry* entries = ALLOCATE(Entry, capacity);
    for (unsigned int i = 0; i < capacity; i++) {
        entries[i].key   = EMPTY_VAL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for (unsigned int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (IS_EMPTY(entry->key))
            continue;

        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key   = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries  = entries;
    table->capacity = capacity;
}

bool tableSet(Table* table, Value key, Value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry* entry    = findEntry(table->entries, table->capacity, key);
    bool   isNewKey = IS_EMPTY(entry->key);
    if (isNewKey && IS_NIL(entry->value))
        table->count++;

    entry->key   = key;
    entry->value = value;
    return isNewKey;
}

bool tableDelete(Table* table, Value key)
{
    if (table->count == 0)
        return false;

    // Find the entry.
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (IS_EMPTY(entry->key))
        return false;

    // Place a tombstone in the entry.
    entry->key   = EMPTY_VAL;
    entry->value = BOOL_VAL(true);
    table->count--;
    return true;
}

void tableAddAll(Table* from, Table* to)
{
    for (unsigned int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (!IS_EMPTY(entry->key)) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash)
{
    if (table->count == 0)
        return NULL;

    uint32_t index = hash & (table->capacity - 1);
    for (;;) {
        Entry* entry = &table->entries[index];

        if (IS_EMPTY(entry->key)) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry->value))
                return NULL;
        }

        ObjString* string = AS_STRING(entry->key);
        if (string->length == length && memcmp(string->chars, chars, length) == 0) {
            // We found it.
            return string;
        }

        index = (index + 1) & (table->capacity - 1);
    }
}

static void printEntry(Entry* entry)
{
    if (IS_EMPTY(entry->key)) {
        printf("<empty>");
        return;
    }

    printf("%s => ", stringValue(entry->key));
    printValue(entry->value);
}

void printTable(Table* table)
{
    printf("{ ");
    for (unsigned int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        Entry* next  = &table->entries[i + 1];
        if (!IS_EMPTY(entry->key)) {
            printEntry(entry);
            if (!IS_EMPTY(next->key))
                printf(", ");
        }
    }
    printf(" }");
}

void tableRemoveWhite(Table* table)
{
    for (unsigned int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (IS_EMPTY(entry->key)) {
            tableDelete(table, entry->key);
        }
    }
}

void markTable(Table* table)
{
    for (unsigned int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        markValue(entry->key);
        markValue(entry->value);
    }
}
