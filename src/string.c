#include "ph_string.h"

char* substring_utf8(utf8_int8_t* src, int start, int end)
{
    char* dst     = (char*)malloc(strlen(src) + 1); // Allocating maximum possible size
    char* dst_ptr = dst;
    int   index   = 0;

    utf8_int32_t codepoint;
    void*        next = (void*)src;
    while (next) {
        next = utf8codepoint(next, &codepoint);
        if (index >= start && index < end) {
            dst_ptr = utf8catcodepoint(dst_ptr, codepoint, strlen(src) - (dst_ptr - dst));
        }
        if (index >= end) {
            break;
        }
        index++;
    }
    *dst_ptr = '\0';
    return dst;
}

utf8_int8_t* replace_utf8(utf8_int8_t* original, utf8_int8_t* from, utf8_int8_t* to)
{
    utf8_int8_t* result;
    utf8_int8_t* ins;
    utf8_int8_t* tmp;
    int          len_from;
    int          len_to;
    int          len_front;
    int          count;

    len_from = strlen(from);
    if (len_from == 0)
        return NULL;

    len_to = strlen(to);
    if (len_to == 0)
        return NULL;

    ins = original;

    for (count = 0; (tmp = strstr(ins, from)); ++count) {
        ins = tmp + len_from;
    }

    tmp = result = malloc(strlen(original) + (len_to - len_from) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins       = strstr(original, from);
        len_front = ins - original;
        tmp       = strncpy(tmp, original, len_front) + len_front;
        tmp       = strcpy(tmp, to) + len_to;
        original += len_front + len_from;
    }
    strcpy(tmp, original);
    return result;
}

void split_utf8(utf8_int8_t* src, utf8_int8_t* delim, void*** out_tokens, size_t* out_token_count)
{
    utf8_int8_t* src_ptr   = (utf8_int8_t*)src;
    utf8_int8_t* delim_ptr = (utf8_int8_t*)delim;
    size_t       src_len   = strlen(src_ptr);
    size_t       delim_len = strlen(delim_ptr);

    // Count the number of occurrences of the delimiter in the source string
    size_t       count = 0;
    utf8_int8_t* ptr   = src_ptr;
    while ((ptr = utf8str(ptr, delim_ptr)) != NULL) {
        count++;
        ptr += delim_len;
    }

    // Allocate memory for the output token array
    void** tokens = malloc((count + 1) * sizeof(void*));
    if (tokens == NULL) {
        // Error handling: Memory allocation failed
        // Handle the error according to your requirements
        *out_tokens      = NULL;
        *out_token_count = 0;
        return;
    }

    size_t token_index      = 0;
    ptr                     = src_ptr;
    utf8_int8_t* next_delim = utf8str(ptr, delim_ptr);

    while (next_delim != NULL) {
        size_t token_len = next_delim - ptr;

        // Allocate memory for the current token
        void* token = malloc(token_len + 1); // +1 for null terminator
        if (token == NULL) {
            // Error handling: Memory allocation failed
            // Handle the error according to your requirements
            // Clean up and return
            for (size_t i = 0; i < token_index; i++) {
                free(tokens[i]);
            }
            free(tokens);
            *out_tokens      = NULL;
            *out_token_count = 0;
            return;
        }

        // Copy the token to the allocated memory
        memcpy(token, ptr, token_len);
        *((unsigned char*)token + token_len) = '\0'; // Add null terminator

        tokens[token_index] = token;
        token_index++;

        ptr        = next_delim + delim_len;
        next_delim = utf8str(ptr, delim_ptr);
    }

    // Handle the remaining part after the last delimiter
    size_t remaining_len   = src_ptr + src_len - ptr;
    void*  remaining_token = malloc(remaining_len + 1); // +1 for null terminator
    if (remaining_token == NULL) {
        // Error handling: Memory allocation failed
        // Handle the error according to your requirements
        // Clean up and return
        for (size_t i = 0; i < token_index; i++) {
            free(tokens[i]);
        }
        free(tokens);
        *out_tokens      = NULL;
        *out_token_count = 0;
        return;
    }

    memcpy(remaining_token, ptr, remaining_len);
    *((unsigned char*)remaining_token + remaining_len) = '\0'; // Add null terminator

    tokens[token_index] = remaining_token;
    token_index++;

    *out_tokens      = tokens;
    *out_token_count = token_index;
}

// TODO: Not actually UTF-8
void trim_utf8(utf8_int8_t* src, utf8_int8_t* trim)
{
    int trimlen = strlen(trim);
    int len     = strlen(src);
    int start   = 0;
    int end     = len - 1;

    // Find the first trim match from the beginning
    for (int i = 0; i < len; i++) {
        int found = 0;
        for (int j = 0; j < trimlen; j++) {
            if (src[i] == trim[j]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            start = i;
            break;
        }
    }

    // Find the last trim match from the end
    for (int i = len - 1; i >= 0; i--) {
        int found = 0;
        for (int j = 0; j < trimlen; j++) {
            if (src[i] == trim[j]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            end = i;
            break;
        }
    }

    // Shift the remaining characters to the beginning of the string
    int shift = 0;
    for (int i = start; i <= end; i++) {
        src[i - start] = src[i];
        shift++;
    }

    // Null-terminate the trimmed string
    src[shift] = '\0';
}

utf8_int8_t* reverse_utf8(utf8_int8_t* src)
{
    size_t       len = utf8len(src);
    utf8_int8_t* rev = malloc(strlen(src) + 1);

    utf8_int32_t codepoint;
    utf8_int8_t* next = src + strlen(src) - 1;
    utf8_int8_t* tmp  = rev;
    for (size_t i = 0; i < len; i++) {
        next = utf8rcodepoint(next, &codepoint);
        tmp  = utf8catcodepoint(tmp, codepoint, utf8codepointsize(codepoint));
    }

    rev[strlen(src)] = '\0';
    return rev;
}

utf8_int8_t* repeat_utf8(utf8_int8_t* src, size_t count)
{
    size_t       len = strlen(src);
    utf8_int8_t* rep = malloc(len * count + 1);

    for (size_t i = 0; i < count; i++) {
        strcat(rep, src);
    }

    return rep;
}

char* replace_placeholder(char* template, char* value)
{
    char* ptr = strstr(template, "{}");
    if (ptr != NULL) {
        char   buffer[strlen(template) + TEMPLATE_BUFFER];
        size_t len = ptr - template;
        strncpy(buffer, template, len);
        buffer[len] = '\0';
        strcat(buffer, value);
        strcat(buffer, ptr + 2);
        strcpy(template, buffer);
    }
    return template;
}
