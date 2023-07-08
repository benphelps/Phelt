#include "native/debug.h"

bool debug_frame(int argCount, Value* args)
{
    phelt_checkArgs(1);
    phelt_checkNumber(0);
    int depth = (int)phelt_toNumber(0);

    if (vm.frameCount == 0) {
        // impossible
        phelt_error("No frames to display.");
        return false;
    }

    if (depth < 0 || depth > vm.frameCount) {
        phelt_error("Invalid frame depth.");
        return false;
    }

    CallFrame*   frame    = &vm.frames[vm.frameCount - 1 - depth];
    ObjClosure*  closure  = frame->closure;
    ObjFunction* function = closure->function;

    ObjTable* table = newTable();

    tableSet(
        &table->table,
        OBJ_VAL(copyString("source", 6)),
        OBJ_VAL(copyString(function->source, (int)strlen(function->source))));

    tableSet(
        &table->table,
        OBJ_VAL(copyString("line", 4)),
        function->chunk.lines.values[frame->ip - function->chunk.code - 1]);

    ObjTable* funTable = newTable();

    Value name = function->name ? OBJ_VAL(function->name) : NIL_VAL;

    tableSet(
        &funTable->table,
        OBJ_VAL(copyString("line", 4)),
        NUMBER_VAL(function->line));

    tableSet(
        &funTable->table,
        OBJ_VAL(copyString("name", 4)),
        name);

    tableSet(
        &funTable->table,
        OBJ_VAL(copyString("arity", 5)),
        NUMBER_VAL(function->arity));

    tableSet(
        &table->table,
        OBJ_VAL(copyString("function", 8)),
        OBJ_VAL(funTable));

    phelt_pushObject(-1, table);
    return true;
}
