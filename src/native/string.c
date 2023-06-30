#include "native/string.h"

bool string_length(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkString(0);

    ObjString* string = phelt_toString(0);
    phelt_pushNumber(-1, string->length);
    return true;
}

bool string_sub(int argCount, Value* args)
{
    phelt_checkArgs(3);
    phelt_checkString(0);
    phelt_checkNumber(1);
    phelt_checkNumber(2);

    char*    string     = phelt_toCString(0);
    uint32_t start      = (uint32_t)phelt_toNumber(1);
    uint32_t end        = (uint32_t)phelt_toNumber(2);
    uint32_t str_length = utf8len(string);

    if (start < 0)
        start = (str_length + 1) + start;

    if (end < 0)
        end = (str_length + 1) + end;

    if (start < 0 || start >= str_length || end < 0 || end >= str_length) {
        phelt_error("String index out of bounds");
        return false;
    }

    char* sub = substring_utf8(string, start, end);
    phelt_pushString(-1, copyString(sub, strlen(sub)));
    free(sub);
    return true;
}

bool string_find(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkString(0);
    phelt_checkString(1);

    char* string = phelt_toCString(0);
    char* search = phelt_toCString(1);

    utf8_int8_t* found = utf8str(string, search);
    if (found == NULL) {
        phelt_pushNumber(-1, -1);
        return true;
    }

    int index = utf8len(found);
    phelt_pushNumber(-1, index);
    return true;
}

bool string_replace(int argCount, Value* args)
{
    phelt_checkArgs(3);
    phelt_checkString(0);
    phelt_checkString(1);
    phelt_checkString(2);

    char* string  = phelt_toCString(0);
    char* search  = phelt_toCString(1);
    char* replace = phelt_toCString(2);

    char* replaced = replace_utf8(string, search, replace);
    phelt_pushString(-1, takeString(replaced, strlen(replaced)));
    return true;
}

// split_utf8(const void* src, const void* delim, void*** out_tokens, size_t* out_token_count);
bool string_split(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkString(0);
    phelt_checkString(1);

    char* string = phelt_toCString(0);
    char* split  = phelt_toCString(1);

    void** tokens;
    size_t token_count;
    split_utf8(string, split, &tokens, &token_count);

    ObjArray* array = newArray();

    for (size_t i = 0; i < token_count; i++) {
        ObjString* token = takeString(tokens[i], strlen(tokens[i]));
        writeValueArray(&array->array, OBJ_VAL(token));
    }

    free(tokens);
    phelt_pushObject(-1, array);
    return true;
}

bool string_trim(int argCount, Value* args)
{
    phelt_checkArgs(2);
    phelt_checkString(0);
    phelt_checkString(1);

    char* string = phelt_toCString(0);
    char* trim   = phelt_toCString(1);
    char* copy   = copyStringRaw(string, strlen(string));
    trim_utf8(copy, trim);
    phelt_pushString(-1, takeString(copy, strlen(copy)));
    return true;
}

bool string_upper(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkString(0);

    char* string = phelt_toCString(0);
    char* copy   = copyStringRaw(string, strlen(string));
    utf8upr(copy);
    phelt_pushString(-1, takeString(copy, strlen(copy)));
    return true;
}

bool string_lower(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkString(0);

    char* string = phelt_toCString(0);
    char* copy   = copyStringRaw(string, strlen(string));
    utf8lwr(copy);
    phelt_pushString(-1, takeString(copy, strlen(copy)));
    return true;
}

bool string_reverse(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkString(0);

    char* string = phelt_toCString(0);
    char* rev    = reverse_utf8(string);
    phelt_pushString(-1, takeString(rev, strlen(rev)));
    return true;
}
