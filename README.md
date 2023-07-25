# Phelt

Phelt is a small and versatile scripting language. It's based on Lox, as described in the second part of [Crafting Interpreters](https://craftinginterpreters.com/), with many additions and changes.

# Features

-   Bytecode Optimizations
    -   Constant folding (compile time arithmetic)
    -   Local & Global access fusion (consecutive local or global access instructions are merged into a single instruction)
    -   `CALL` instructions that immediately throw away the result are converted to `CALL_BLIND`
    -   Consecutive `POP`s are merged into a single `POP_N` with the count as the operand
-   UTF-8 support
    -   Strings & identifiers, literals, function names, class names, etc
    -   It should "just work" everywhere, including indexing and slicing operations
-   Variables
    -   Strings (immutable), Numbers (floats, integers, hex, binary, octal), Booleans, Nil
        -   Single, double, triple quoted strings. Triple quoted strings act as heredocs, and can span multiple lines, and require no escaping.
        -   All numbers are internally represented as doubles, cast to integers when required.
        -   Equality is direct, no type coercion, `0` does not equal `false`.
    -   Arrays, Tables (dictionaries, maps, etc., whatever you want to call them)
        -   Keys can be any type
        -   Values can be any type
    -   Slices & negative indices for types where it makes sense (eg. `[2..-2]`)
-   Control flow
    -   if / else if / else
    -   while, do while, for
    -   switch (non-fallthrough)
-   Functions
    -   Named functions, Anonymous functions
    -   Inline single expression functions
    -   Closures
-   Classes
    -   Constructors, Inheritance
    -   Defined properties (not yet constants)
    -   Dunder methods (e.g. `__str`, `__add`, `__sub`, `__eq`, etc.)
-   Mostly reentrant
    -   The VM is mostly reentrant, you can invoke Phelt functions from C, this is how `__str` is implemented for example.
-   Standard library
    -   The standard library is written in C, and is compiled into the `phelt` executable.
        -   A lua-esque set of macros for manipulating the stack, allows for easy implementation of functions in C
    -   It is a work in progress, might be buggy
    -   Currently includes `system`, `math`, `http`, `file`, `array`, `table`, `json` and `debug` modules
    -   On-demand loading of modules, using `module(name)` function
        -   Keeps namespace clean, allows for mapping modules to your own names
-   Imports
    -   Imports are done using the `import` keyword
    -   Imports share the same namespace as the file they are imported into
-   Visual Studio Code Extension
    -   Syntax highlighting extension is available here: [phelt - language](https://github.com/benphelps/phelt-language)

# Compiling

Phelt is written in C, and builds with Cmake. To compile Phelt, run the following commands:

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

If modifications are made to the `identifiers.gperf` file, the following command must be run to regenerate the `identifiers.def` file:

```bash
gperf identifiers.gperf > identifiers.def
```

# Usage

To run a Phelt script, use the `phelt` command:

```bash
phelt script.ph
```

To run the REPL, use the `phelt` command without any arguments:

```bash
phelt
```

# Examples

## Hello World

```js
print("Hello World!");
```

## UTF-8

```js
let string = "Hello World";
let приклад = "phelt говорить вашою мовою";
let サンプル = "pheltはあなたの言語を話します";
let उदाहरण = "लक्स आपकी भाषा बोलता है";
let 💎 = "🙌";
```

## Variables

```js
let string = "Hello World";
let float = 12.35;
let hex = 0x10 + 0x20;
let binary = 0b10 + 0b11;
let octal = 0o20 + 0o10;
let truthy = true;
let falsy = false;
let nadda = nil;
let array = [1, 2, 3] + [4, 5, 6];
let table = { a: 1, b: 2, c: 3 } + { d: 4, e: 5, f: 6 };
```

## Multiple Assignment

The number of variables on the left must match the number of values on the right, 

```js
let a, b, c = 1, 2, 3;
let x, y, z;
```

## Control Flow

```js
if (true) {
    print("true");
} else if (false) {
    print("false");
} else {
    print("nadda");
}

while (true) {
    print("true");
    break;
}

do {
    print("once");
} while (false);

for (let i = 0; i < 10; i = i + 1) {
    print(i);
}

switch (1) {
    case 1:
    // one
    case 2:
    // two
    default:
    // default
}
```

## Functions

```js
fun add(a, b) {
  return a + b;
}

// inline, single expression, implicit return
fun add(a, b) a + b;
```

## Closures

```js
fun makeCounter() {
  let i = 0;
  return fun() {
    i = i + 1;
    return i;
  };
}

let counter = makeCounter();
println(counter()); // 1
println(counter()); // 2
println(counter()); // 3
```

## Classes

```js
class Person {
    init(name) {
        this.name = name;
    }

    greet() {
        println("Hello, my name is {}", this.name);
    }
}

let person = Person("John");
person.greet(); // Hello, my name is John
```

## Inheritance

```js
class Person {
  init(name) {
    this.name = name;
  }

  greet() {
    println("Hello, my name is {}", this.name);
  }
}

class Employee < Person {
  init(name, id) {
    super.init(name);
    this.id = id;
  }

  greet() {
    super.greet();
    println("My employee id is {}", this.id);
  }
}

let employee = Employee("John", 123);
employee.greet(); // Hello, my name is John
                  // My employee id is 123
```

## Properties

```js
class Base {
    let explicitProperty = "explicit";

    method() {
        this.implicitProperty = "implicit";
    }
}
```

## Dunder Methods

Currently supported dunder methods:

-   `__str` — called when printed, or otherwise converted to a string
-   `__add`, `__sub`, `__mul`, `__div` — called when using `+`, `-`, `*`, `/` operators
-   `__gt`, `__lt`, `__eq`, `__gte`, `__lte`, `__ne`, `__not` — called when using `>`, `<`, `==`, `>=`, `<=`, `!=`, `!` operators
-   `__and`, `__or`, `__xor`, `__mod` — called when using `&`, `|`, `^`, `%` operators
-   `__rshift`, `__lshift` — called when using `>>`, `<<` operators

```js
class Vector {
    init(x, y) {
        this.x = x;
        this.y = y;
    }

    __add(other) Vector(this.x + other.x, this.y + other.y);
    __sub(other) Vector(this.x - other.x, this.y - other.y);

    dot(other) this.x * other.x + this.y * other.y;
    scale(s) Vector(this.x * s, this.y * s);

    reflect(other) {
        return this - other.scale(2 * this.dot(other) / other.dot(other));
    }
}
```

# Standard Library

The standard library is a work in progress. It is written in C, and is compiled into the `phelt` executable, so it is always available.

These are functions that are always available in the global scope.

```js
print("Hello World"); // no newline
println("Hello World"); // with newline
let stype = typeof(value); // "number"

let http = module("http"); // loads the http module
```

Other modules can be loaded with the `module` function, when required.

## `system`

```js
let system = module("system");

let formatted = sprint("Hello {}", "World"); // "Hello World"
let time = system.time(); // seconds since epoch
let mtime = system.mtime(); // milliseconds since epoch
let clock = system.clock(); // ms since process start
system.sleep(1); // sleep for 1 second
system.usleep(100); // sleep for 100 ms
```

## `math`

```js
let math = module("math");

let ceil = math.ceil(1.5);
let floor = math.floor(1.5);
let abs = math.abs(-1.5);
let exp = math.exp(12);
let sqrt = math.sqrt(4);
let sin = math.sin(0);
let cos = math.cos(0);
let tan = math.tan(0);
let atan = math.atan(0);
let pow = math.pow(2, 3);
let atan2 = math.atan2(1, 1);
let deg = math.deg(rad);
let rad = math.rad(deg);
let clamp = math.clamp(value, min, max);
let lerp = math.lerp(start, end, pos);
let map = math.map(value, startA, startB, endA, endB);
let norm = math.norm(value, start, stop);
let round = math.round(value, precision);

math.seed(seed); // seed the random number generator
let rand = math.rand(); // random float between 0 and 1

// along with the following constants
let e = math.E;
let log2e = math.LOG2E;
let log10e = math.LOG10E;
let ln2 = math.LN2;
let ln10 = math.LN10;
let pi = math.PI;
let tau = math.TAU;
let sqrt2 = math.SQRT2;
let sqrt1_2 = math.SQRT1_2;
```

## `http`

This is a very basic HTTP client. It is synchronous, and only supports GET, POST, PUT, DELETE, PATCH, HEAD, and OPTIONS. It always returns the response body, or raw http response in the case of HEAD and OPTIONS.

```js
let http = module("http");

let response = http.get("http://localhost/get");
let response = http.post("http://localhost/post", "hello=world");
let response = http.put("http://localhost/put", "hello=world");
let response = http.delete("http://localhost/delete", "hello=world");
let response = http.patch("http://localhost/patch", "hello=world");
let response = http.head("http://localhost/get");
let response = http.options("http://localhost/get");
```

## `file`

These are more or less wrappers around the C standard library file functions.

```js
let file = module("file");

// std streams, valid fp objects
let stdin = file.stdin;
let stdout = file.stdout;
let stderr = file.stderr;

// constants
let SEEK_SET = file.SEEK_SET;
let SEEK_CUR = file.SEEK_CUR;
let SEEK_END = file.SEEK_END;

// open a file
let fp = file.open("file.txt", "w+");
let fp = file.tmpfile();
let fp = file.mkstemps("fileXXXXXX");

// close a file
file.close(fp);

// write to a file
let bytes_written = file.write(fp, data);
file.putc(fp, 65);
file.puts(fp, "Hello World");
file.flush(fp);

// read from a file
let data = file.read(fp, bytes);
let char = file.getc(fp);
let data = file.gets(fp, 10);

// traverse a file
let pos = file.seek(fp, 0, SEEK_END);
let pos = file.tell(fp);
file.rewind(fp);

// file operations
file.remove("test.txt");
file.rename("test.txt", "test2.txt");
```

## `array`

```js
let array = module("array");

let length = array.length(arr);
array.push(arr, value);
let popped = array.pop(arr);
array.insert(a, idx, value);
let removed = array.remove(a, idx);
array.sort(arr, fun(a, b) a > b);
array.reverse(arr);
let firstIndex = array.find(arr, idx);
let lastIndex = array.findLast(arr, idx);
let mapped = array.map(arr, func);
let filtered = array.filter(mapped, func);
let reduced = array.reduce(filtered, func, acc);
let flattened = array.flatten(arr);
```

## `table`

```js
let table = module("table");

table.length(tbl);
table.keys(tbl);
table.values(tbl);
table.remove(tbl, key);
table.insert(tbl, key, value);
table.hasKey(tbl, key);
```

## `string`

```js
let string = module("string");

let new = string.length(str);
let new = string.upper(str);
let new = string.lower(str);
let new = string.split(str, delim);
let new = string.trim(str, chars);
let new = string.sub(str, start, end);
let new = string.find(str, substr);
let new = string.replace(str, substr, newstr);
let new = string.reverse(str);
let new = string.repeat(str, count);
```

## `json`

```js
let json = module("json");

let object = json.decode(input [, isSimplifiedJSON]);
let string = json.encode(object);
```

## `debug`

```js
let debug = module("debug");

fun test() {
    let self = debug.frame(0);
    let caller = debug.frame(1);

    println(
        "I am fn `{}` defined on line, I take {} arguments, {} called from line {} in {}",
        self.function.name, self.function.line, self.function.arity,
        caller.line, caller.source
    );
}

test();
```
