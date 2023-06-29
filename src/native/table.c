#include "native/table.h"

bool table_length(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkTable(0);

    ObjTable* table = phelt_toTable(0);
    phelt_pushNumber(-1, table->table.count);
    return true;
}

bool table_keys(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkTable(0);

    ObjTable* table = phelt_toTable(0);
    ObjArray* array = newArray();
    for (int i = 0; i < table->table.capacity; i++) {
        Entry* entry = &table->table.entries[i];
        if (!IS_EMPTY(entry->key)) {
            writeValueArray(&array->array, entry->key);
        }
    }
    phelt_pushObject(-1, (Obj*)array);
    return true;
}

bool table_values(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkTable(0);

    ObjTable* table = phelt_toTable(0);
    ObjArray* array = newArray();
    for (int i = 0; i < table->table.capacity; i++) {
        Entry* entry = &table->table.entries[i];
        if (!IS_EMPTY(entry->key)) {
            writeValueArray(&array->array, entry->value);
        }
    }
    phelt_pushObject(-1, (Obj*)array);
    return true;
}

bool table_hasKey(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkTable(0);

    ObjTable* table = phelt_toTable(0);
    phelt_pushBool(-1, tableGet(&table->table, phelt_value(1), NULL));
    return true;
}

bool table_remove(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkTable(0);

    ObjTable* table = phelt_toTable(0);
    phelt_pushBool(-1, tableDelete(&table->table, phelt_value(1)));
    return true;
}

bool table_insert(int argCount, Value* args)
{
    phelt_checkArgs(3);
    phelt_checkTable(0);

    ObjTable* table = phelt_toTable(0);
    tableSet(&table->table, phelt_value(1), phelt_value(2));
    return true;
}
