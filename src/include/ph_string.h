#ifndef PHELT_STRING_H
#define PHELT_STRING_H

#include "common.h"
#include "utf8.h"
#include <ctype.h>

utf8_int8_t* substring_utf8(utf8_int8_t* src, int start, int end);
utf8_int8_t* replace_utf8(utf8_int8_t* str, utf8_int8_t* old_sub, utf8_int8_t* new_sub);
utf8_int8_t* reverse_utf8(utf8_int8_t* str);
void         trim_utf8(utf8_int8_t* src, utf8_int8_t* trim);
void         split_utf8(utf8_int8_t* src, utf8_int8_t* delim, void*** out_tokens, size_t* out_token_count);
char*        replace_placeholder(char* template, char* value);

#endif
