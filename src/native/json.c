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

bool json_decode(int argCount, Value* args)
{
    enum json_parse_flags_e flags = json_parse_flags_allow_json5;

    phelt_checkMinArgs(1);
    phelt_checkString(0);

    if (argCount == 2) {
        phelt_checkBool(1);
        if (phelt_toBool(1)) {
            flags = json_parse_flags_allow_simplified_json | json_parse_flags_allow_c_style_comments;
        }
    }

    char* json = phelt_toCString(0);

    struct json_value_s* root = json_parse_ex(json, strlen(json), flags, NULL, NULL, NULL);

    if (root == NULL) {
        phelt_error("Invalid JSON.");
        return false;
    }

    if (root->type == json_type_object) {
        struct json_object_s* object = (struct json_object_s*)root->payload;
        phelt_pushObject(-1, json_object_to_table(object));
    } else if (root->type == json_type_array) {
        struct json_array_s* array = (struct json_array_s*)root->payload;
        phelt_pushObject(-1, json_array_to_array(array));
    } else if (root->type == json_type_string) {
        char* value = root->payload;
        phelt_pushString(-1, copyString(value, strlen(value)));
    } else if (root->type == json_type_number) {
        double value = atoll(json_value_as_number(root)->number);
        phelt_pushNumber(-1, value);
    } else if (root->type == json_type_true) {
        phelt_pushBool(-1, true);
    } else if (root->type == json_type_false) {
        phelt_pushBool(-1, false);
    } else if (root->type == json_type_null) {
        phelt_pushNil(-1);
    } else {
        phelt_error("Invalid JSON.");
        return false;
    }

    free(root);
    return true;
}

char* array_to_string(ObjArray* array);
char* table_to_string(ObjTable* table);

char* escape_json_string(char* string)
{
    char* buffer = malloc(1);
    buffer[0]    = '\0';

    for (unsigned int i = 0; i < strlen(string); i++) {
        char* temp;

        switch (string[i]) {
        case '"':
            temp = malloc(strlen(buffer) + 3);
            sprintf(temp, "%s\\\"", buffer);
            break;
        case '\\':
            temp = malloc(strlen(buffer) + 3);
            sprintf(temp, "%s\\\\", buffer);
            break;
        case '\b':
            temp = malloc(strlen(buffer) + 3);
            sprintf(temp, "%s\\b", buffer);
            break;
        case '\f':
            temp = malloc(strlen(buffer) + 3);
            sprintf(temp, "%s\\f", buffer);
            break;
        case '\n':
            temp = malloc(strlen(buffer) + 3);
            sprintf(temp, "%s\\n", buffer);
            break;
        case '\r':
            temp = malloc(strlen(buffer) + 3);
            sprintf(temp, "%s\\r", buffer);
            break;
        case '\t':
            temp = malloc(strlen(buffer) + 3);
            sprintf(temp, "%s\\t", buffer);
            break;
        default:
            temp = malloc(strlen(buffer) + 2);
            sprintf(temp, "%s%c", buffer, string[i]);
            break;
        }

        free(buffer);
        buffer = temp;
    }

    return buffer;
}

char* array_to_string(ObjArray* object)
{
    ValueArray array = object->array;

    char* buffer = malloc(1);
    buffer[0]    = '\0';

    for (unsigned int i = 0; i < array.count; i++) {
        Value value = array.values[i];
        char* temp;

        if (IS_STRING(value)) {
            char* string = escape_json_string(AS_CSTRING(value));
            temp         = malloc(strlen(buffer) + strlen(string) + 5);
            sprintf(temp, "%s\"%s\",", buffer, string);
        } else if (IS_NUMBER(value)) {
            double number = AS_NUMBER(value);
            temp          = malloc(strlen(buffer) + 32);
            sprintf(temp, "%s%g,", buffer, number);
        } else if (IS_BOOL(value)) {
            bool boolean = AS_BOOL(value);
            temp         = malloc(strlen(buffer) + 32);
            sprintf(temp, "%s%s,", buffer, boolean ? "true" : "false");
        } else if (IS_NIL(value)) {
            temp = malloc(strlen(buffer) + 32);
            sprintf(temp, "%s%s,", buffer, "null");
        } else if (IS_OBJ(value)) {
            if (IS_ARRAY(value)) {
                temp = array_to_string(AS_ARRAY(value));
            } else if (IS_TABLE(value)) {
                temp = table_to_string(AS_TABLE(value));
            } else {
                temp = malloc(strlen(buffer) + 32);
                sprintf(temp, "%s%s,", buffer, "null");
            }
        } else {
            temp = malloc(strlen(buffer) + 32);
            sprintf(temp, "%s%s,", buffer, "null");
        }

        free(buffer);
        buffer = temp;
    }

    // Remove the trailing comma.
    buffer[strlen(buffer) - 1] = '\0';

    // wrap in brackets
    char* temp = malloc(strlen(buffer) + 3);
    sprintf(temp, "[%s]", buffer);
    free(buffer);
    buffer = temp;

    return buffer;
}

char* table_to_string(ObjTable* object)
{
    Table table = object->table;

    char* buffer = malloc(1);
    buffer[0]    = '\0';

    for (unsigned int i = 0; i < table.capacity; i++) {
        Entry* entry = &table.entries[i];
        if (!IS_EMPTY(entry->key)) {
            char* key = AS_CSTRING(entry->key);
            char* value;

            if (IS_STRING(entry->value)) {
                value      = escape_json_string(AS_CSTRING(entry->value));
                char* temp = malloc(strlen(value) + 3);
                sprintf(temp, "\"%s\"", value);
                free(value);
                value = temp;
            } else if (IS_NUMBER(entry->value)) {
                value = malloc(1);
                sprintf(value, "%g", AS_NUMBER(entry->value));
            } else if (IS_BOOL(entry->value)) {
                value = malloc(1);
                sprintf(value, "%s", AS_BOOL(entry->value) ? "true" : "false");
            } else if (IS_NIL(entry->value)) {
                value = malloc(1);
                sprintf(value, "%s", "null");
            } else if (IS_OBJ(entry->value)) {
                if (IS_ARRAY(entry->value)) {
                    value = array_to_string(AS_ARRAY(entry->value));
                } else if (IS_TABLE(entry->value)) {
                    value = table_to_string(AS_TABLE(entry->value));
                } else {
                    value = malloc(1);
                    sprintf(value, "%s", "null");
                }
            } else {
                value = malloc(1);
                sprintf(value, "%s", "null");
            }

            char* temp = malloc(strlen(buffer) + strlen(key) + strlen(value) + 5);
            sprintf(temp, "%s\"%s\":%s,", buffer, key, value);
            free(buffer);
            buffer = temp;
        }
    }

    // Remove the trailing comma.
    buffer[strlen(buffer) - 1] = '\0';

    // wrap in braces
    char* temp = malloc(strlen(buffer) + 3);
    sprintf(temp, "{%s}", buffer);
    free(buffer);
    buffer = temp;

    return buffer;
}

bool json_encode(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkObject(0);

    Obj* object = AS_OBJ(phelt_value(0));

    if (object->type == OBJ_ARRAY) {
        ObjArray* array = (ObjArray*)object;
        char*     value = array_to_string(array);
        phelt_pushCString(-1, value);
        free(value);
    } else if (object->type == OBJ_TABLE) {
        ObjTable* table = (ObjTable*)object;
        char*     value = table_to_string(table);
        phelt_pushCString(-1, value);
        free(value);
    } else {
        phelt_error("Invalid object type.");
        return false;
    }

    return true;
}
