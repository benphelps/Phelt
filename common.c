#include "common.h"
#include <unistd.h>

utf8_int8_t* readFile(const char* path)
{
    const char* resolvedPath = resolvePath(path);

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    utf8_int8_t* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

const char* resolvePath(const char* path)
{
    char* cwd          = getcwd(NULL, 0);
    char* resolvedPath = (char*)malloc(strlen(cwd) + strlen(path) + 2);
    strcpy(resolvedPath, cwd);
    strcat(resolvedPath, "/");
    strcat(resolvedPath, path);
    free(cwd);
    return resolvedPath;
}

// get the path for a file
const char* getFilePath(const char* path)
{
    char* resolvedPath = (char*)malloc(strlen(path) + 1);
    strcpy(resolvedPath, path);
    char* lastSlash = strrchr(resolvedPath, '/');
    if (lastSlash == NULL) {
        free(resolvedPath);
        return NULL;
    }
    *lastSlash = '\0';
    return resolvedPath;
}

// check for a file relative to the current file, or in the current directory
const char* resolveRelativePath(const char* path, const char* currentFile)
{
    const char* filePath = getFilePath(currentFile);
    if (filePath == NULL) {
        return resolvePath(path);
    }
    char* resolvedPath = (char*)malloc(strlen(filePath) + strlen(path) + 2);
    strcpy(resolvedPath, filePath);
    strcat(resolvedPath, "/");
    strcat(resolvedPath, path);
    free((void*)filePath);
    return resolvedPath;
}

bool fileExists(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    fclose(file);
    return true;
}
