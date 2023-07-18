#include "native/json.h"

ObjTable* json_object_to_table(json_object_t* object);
ObjArray* json_array_to_array(json_array_t* array);

ObjArray* json_array_to_array(json_array_t* array)
{
    ObjArray* objArray = newArray();

    struct json_array_element_s* entry = array->start;

    while (entry != NULL) {
        switch (entry->value->type) {
        case json_type_string: {
            const char* value = json_value_as_string(entry->value)->string;

            writeValueArray(&objArray->array, OBJ_VAL(copyString(value, strlen(value))));
            break;
        }
        case json_type_number: {
            double value = atoll(json_value_as_number(entry->value)->number);
            writeValueArray(&objArray->array, NUMBER_VAL(value));
            break;
        }
        case json_type_object: {
            json_object_t* value = entry->value->payload;

            writeValueArray(&objArray->array, OBJ_VAL(json_object_to_table(value)));
            break;
        }
        case json_type_array: {
            json_array_t* value = entry->value->payload;

            writeValueArray(&objArray->array, OBJ_VAL(json_array_to_array(value)));
            break;
        }
        case json_type_true: {
            writeValueArray(&objArray->array, BOOL_VAL(true));
            break;
        }
        case json_type_false: {
            writeValueArray(&objArray->array, BOOL_VAL(false));
            break;
        }
        case json_type_null: {
            writeValueArray(&objArray->array, NIL_VAL);
            break;
        }
        }

        entry = entry->next;
    }

    return objArray;
}

ObjTable* json_object_to_table(json_object_t* object)
{
    ObjTable* table = newTable();

    struct json_object_element_s* entry = object->start;

    while (entry != NULL) {
        const char* key = entry->name->string;

        switch (entry->value->type) {
        case json_type_string: {
            const char* value = json_value_as_string(entry->value)->string;

            tableSet(&table->table, OBJ_VAL(copyString(key, strlen(key))), OBJ_VAL(copyString(value, strlen(value))));
            break;
        }
        case json_type_number: {
            double value = atof(json_value_as_number(entry->value)->number);
            tableSet(&table->table, OBJ_VAL(copyString(key, strlen(key))), NUMBER_VAL(value));
            break;
        }
        case json_type_object: {
            json_object_t* value = entry->value->payload;

            tableSet(&table->table, OBJ_VAL(copyString(key, strlen(key))), OBJ_VAL(json_object_to_table(value)));
            break;
        }
        case json_type_array: {
            json_array_t* value = entry->value->payload;

            tableSet(&table->table, OBJ_VAL(copyString(key, strlen(key))), OBJ_VAL(json_array_to_array(value)));
            break;
        }
        case json_type_true: {
            tableSet(&table->table, OBJ_VAL(copyString(key, strlen(key))), BOOL_VAL(true));
            break;
        }
        case json_type_false: {
            tableSet(&table->table, OBJ_VAL(copyString(key, strlen(key))), BOOL_VAL(false));
            break;
        }
        case json_type_null: {
            tableSet(&table->table, OBJ_VAL(copyString(key, strlen(key))), NIL_VAL);
            break;
        }
        }

        entry = entry->next;
    }

    return table;
}

bool _json_parse(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkString(0);

    char* json = phelt_toCString(0);

    struct json_value_s* root = json_parse_ex(json, strlen(json), json_parse_flags_allow_json5, NULL, NULL, NULL);

    if (root == NULL) {
        phelt_pushNil(-1);
        return true;
    }

    if (root->type == json_type_object) {
        struct json_object_s* object = (struct json_object_s*)root->payload;
        phelt_pushObject(-1, json_object_to_table(object));
        return true;
    } else if (root->type == json_type_array) {
        struct json_array_s* array = (struct json_array_s*)root->payload;
        phelt_pushObject(-1, json_array_to_array(array));
        return true;
    } else if (root->type == json_type_string) {
        char* value = root->payload;
        phelt_pushString(-1, copyString(value, strlen(value)));
        return true;
    } else if (root->type == json_type_number) {
        double value = atoll(json_value_as_number(root)->number);
        phelt_pushNumber(-1, value);
        return true;
    } else if (root->type == json_type_true) {
        phelt_pushBool(-1, true);
        return true;
    } else if (root->type == json_type_false) {
        phelt_pushBool(-1, false);
        return true;
    } else if (root->type == json_type_null) {
        phelt_pushNil(-1);
        return true;
    } else {
        phelt_pushNil(-1);
        return true;
    }

    return true;
}
