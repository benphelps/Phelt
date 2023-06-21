# Lux

Lux is a small scripting language, based on Lox, as described in the Crafting Interpreters book.

# Features

-   [x] UTF-8 support
    -   [x] Strings & identifiers
-   [x] Variables
    -   [x] Strings
    -   [x] Numbers (floats, integers, hex, binary, octal)
    -   [x] Booleans
    -   [x] Nil
    -   [x] Arrays
    -   [x] Tables (dictionaries, maps, etc., whatever you want to call them)
        -   [x] Keys can be any type
        -   [x] Values can be any type
-   [x] Control flow
    -   [x] if / else if / else
    -   [x] while
    -   [x] for
    -   [x] switch
-   [x] Functions
    -   [x] Named functions
    -   [x] Anonymous functions
    -   [x] Inline implicit return, single expression functions
    -   [x] Closures
-   [x] Classes
    -   [x] Constructors
    -   [x] Inheritance
    -   [x] Defined properties
    -   [x] Dunder methods (e.g. `__add`, `__sub`, `__eq`, etc.)
-   [x] Standard library

# Compiling

Lux is written in C, and builds with Cmake. To compile Lux, run the following commands:

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
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

## Standard Library

The standard library is a work in progress. The following modules / functions are available:

```js
// math
// ceil, floor, abs, exp, sqrt, sin, cos, tan, atan, pow, atan2, deg, rad, clamp, lerp, map, norm
math.abs(-1);

// system
// time, clock, sleep, usleep
system.clock();

// file
// fopen, fclose, fread, fwrite
let fp = file.fopen("test.txt", "w");
file.fwrite(fp, "Hello World");
file.fclose(fp);
```

See the native modules in the `native` directory for more information.
