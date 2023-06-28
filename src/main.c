#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

static void repl(void)
{
    utf8_int8_t line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret("repl", line);
    }
}

static void runFile(const char* path)
{
    utf8_int8_t*    source = readFile(path);
    InterpretResult result = interpret(path, source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

int main(int argc, const char* argv[])
{
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: lux [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}
