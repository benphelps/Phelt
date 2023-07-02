let debug = module("debug");

fun test() {
    let self = debug.frame(0);
    let caller = debug.frame(1);

    println(
        "I am fn `{}` defined on line {} called from line {} in {}",
        self.function.name, self.function.line,
        caller.line, caller.source
    );
}

test();
