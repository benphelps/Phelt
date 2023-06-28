#include "native/http.h"
#include <curl/curl.h>

const char* useragent = "lux/0.1";

struct string {
    char*  ptr;
    size_t len;
};

void init_string(struct string* s)
{
    s->len = 0;
    s->ptr = malloc(s->len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void* ptr, size_t size, size_t nmemb, struct string* s)
{
    size_t new_len = s->len + size * nmemb;
    s->ptr         = realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len          = new_len;
    return size * nmemb;
}

bool _get(int argCount, Value* args)
{
    lux_checkArgs(1);
    lux_checkString(0);

    CURL* curl = curl_easy_init();
    if (curl) {
        struct string s;
        init_string(&s);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
        curl_easy_setopt(curl, CURLOPT_URL, lux_toCString(0));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            lux_pushObject(-1, formatString("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));
            return false;
        }

        curl_easy_cleanup(curl);

        lux_pushString(-1, copyString(s.ptr, s.len));
        free(s.ptr);
        return true;
    }

    lux_pushObject(-1, formatString("curl_easy_init() failed\n"));
    return false;
}

bool _post(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkString(0);
    lux_checkString(1);

    CURL* curl = curl_easy_init();
    if (curl) {
        struct string s;
        init_string(&s);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
        curl_easy_setopt(curl, CURLOPT_URL, lux_toCString(0));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, lux_toCString(1));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            lux_pushObject(-1, formatString("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));
            return false;
        }

        curl_easy_cleanup(curl);

        lux_pushString(-1, copyString(s.ptr, s.len));
        free(s.ptr);
        return true;
    }

    lux_pushObject(-1, formatString("curl_easy_init() failed\n"));
    return false;
}

bool _put(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkString(0);
    lux_checkString(1);

    CURL* curl = curl_easy_init();
    if (curl) {
        struct string s;
        init_string(&s);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
        curl_easy_setopt(curl, CURLOPT_URL, lux_toCString(0));
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, lux_toCString(1));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            lux_pushObject(-1, formatString("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));
            return false;
        }

        curl_easy_cleanup(curl);

        lux_pushString(-1, copyString(s.ptr, s.len));
        free(s.ptr);
        return true;
    }

    lux_pushObject(-1, formatString("curl_easy_init() failed\n"));
    return false;
}

bool _delete(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkString(0);
    lux_checkString(1);

    CURL* curl = curl_easy_init();
    if (curl) {
        struct string s;
        init_string(&s);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
        curl_easy_setopt(curl, CURLOPT_URL, lux_toCString(0));
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, lux_toCString(1));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            lux_pushObject(-1, formatString("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));
            return false;
        }

        curl_easy_cleanup(curl);

        lux_pushString(-1, copyString(s.ptr, s.len));
        free(s.ptr);
        return true;
    }

    lux_pushObject(-1, formatString("curl_easy_init() failed\n"));
    return false;
}

bool _head(int argCount, Value* args)
{
    lux_checkArgs(1);
    lux_checkString(0);

    CURL* curl = curl_easy_init();
    if (curl) {
        struct string s;
        init_string(&s);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
        curl_easy_setopt(curl, CURLOPT_URL, lux_toCString(0));
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &s);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            lux_pushObject(-1, formatString("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));
            return false;
        }

        curl_easy_cleanup(curl);

        lux_pushString(-1, copyString(s.ptr, s.len));
        free(s.ptr);
        return true;
    }

    lux_pushObject(-1, formatString("curl_easy_init() failed\n"));
    return false;
}

bool _options(int argCount, Value* args)
{
    lux_checkArgs(1);
    lux_checkString(0);

    CURL* curl = curl_easy_init();
    if (curl) {
        struct string s;
        init_string(&s);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
        curl_easy_setopt(curl, CURLOPT_URL, lux_toCString(0));
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &s);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            lux_pushObject(-1, formatString("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));
            return false;
        }

        curl_easy_cleanup(curl);

        lux_pushString(-1, copyString(s.ptr, s.len));
        free(s.ptr);
        return true;
    }

    lux_pushObject(-1, formatString("curl_easy_init() failed\n"));
    return false;
}

bool _patch(int argCount, Value* args)
{
    lux_checkArgs(2);
    lux_checkString(0);
    lux_checkString(1);

    CURL* curl = curl_easy_init();
    if (curl) {
        struct string s;
        init_string(&s);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
        curl_easy_setopt(curl, CURLOPT_URL, lux_toCString(0));
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, lux_toCString(1));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            lux_pushObject(-1, formatString("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));
            return false;
        }

        curl_easy_cleanup(curl);

        lux_pushString(-1, copyString(s.ptr, s.len));
        free(s.ptr);
        return true;
    }

    lux_pushObject(-1, formatString("curl_easy_init() failed\n"));
    return false;
}
