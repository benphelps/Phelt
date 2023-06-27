# Lux - Enlightened scripting

Lux is a small and versitile scripting language, based on Lox, as described in the [Crafting Interpreters](https://craftinginterpreters.com/) book.

# Features

-   UTF-8 support
    -   Strings & identifiers, literals, function names, class names, etc
    -   It should just work everywhere, including indexing, slicing, etc.
-   Variables
    -   Strings, Numbers (floats, integers, hex, binary, octal), Booleans, Nil
    -   Arrays, Tables (dictionaries, maps, etc., whatever you want to call them)
        -   Keys can be any type
        -   Values can be any type
    -   Slices & negative indices for types where it makes sense (eg. `[2..-2]`)
-   Control flow
    -   if / else if / else
    -   while, for
    -   switch
-   Functions
    -   Named functions, Anonymous functions
    -   Inline single expression functions
    -   Closures
-   Classes
    -   Constructors, Inheritance
    -   Defined properties (not yet constants)
    -   Dunder methods (e.g. `__str`, `__add`, `__sub`, `__eq`, etc.)
-   Mostly reentrant
    -   The VM is mostly reentrant, you can invoke Lux functions from C, this is how `__str` is implemented for example.
-   Standard library
    -   The standard library is written in C, and is compiled into the `lux` executable.
        -   A lua-esque set of macros for manipulating the stack, allows for easy implementation of functions in C
    -   It is a work in progress, might be buggy
    -   Currently includes `system`, `math`, `http` and `file` modules
    -   On-demand loading of modules, using `module(name)` function
        -   Keeps namespace clean, allows for mapping modules to your own names
-   Imports
    -   Imports are done using the `import` keyword
    -   Imports share the same namespace as the file they are imported into

# Compiling

Lux is written in C, and builds with Cmake. To compile Lux, run the following commands:

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

To run a Lux script, use the `lux` command:

```bash
lux script.lux
```

To run the REPL, use the `lux` command without any arguments:

```bash
lux
```

# Examples

## Hello World

```js
print("Hello World!");
```

## UTF-8

```js
let string = "Hello World";
let приклад = "lux говорить вашою мовою";
let サンプル = "luxはあなたの言語を話します";
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

## Dunder Methods

Currently supported dunder methods:

-   `__str` — called when printed, or otherwise converted to a string
-   `__add`, `__sub`, `__mul`, `__div` — called when using `+`, `-`, `*`, `/` operators
-   `__gt`, `__lt`, `__eq`, `__not` — called when using `>`, `<`, `==`, `!` operators, other comparison operators are implemented in terms of these
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

The standard library is a work in progress. It is written in C, and is compiled into the `lux` executable, so it is always available.

These are functions that are always available in the global scope.

```js
print("Hello World"); // no newline
println("Hello World"); // with newline
let chars = len("Hello World"); // 11
let count = len([1, 2, 3]); // 3

let http = module("http"); // loads the http module
```

Other modules can be loaded with the `module` function, when required.

## `system`

```js
let system = module("system");

let formatted = sprint("Hello {}", "World"); // "Hello World"
let time = system.time(); // ms since epoch
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
let deg = math.deg(0.7853981633974483);
let rad = math.rad(45);
let clamp = math.clamp(5, 0, 10);
let lerp = math.lerp(0.5, 0, 10);
let map = math.map(5, 0, 10, 0, 100);
let norm = math.norm(5, 0, 10);

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
let fp = fopen("file.txt", "w+");
let fp = file.tmpfile();
let fp = file.mkstemps("fileXXXXXX");

// close a file
fclose(fp);

// write to a file
let bytes_written = fwrite(fp, data);
file.fputc(fp, 65);
file.fputs(fp, "Hello World");
file.fflush(fp);

// read from a file
let data = file.fread(fp, bytes);
let char = file.fgetc(fp);
let data = file.fgets(fp, 10);

// traverse a file
let pos = file.fseek(fp, 0, SEEK_END);
let pos = file.ftell(fp);
file.rewind(fp);

// file operations
file.remove("test.txt");
file.rename("test.txt", "test2.txt");
```
