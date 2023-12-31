/*
block comments
*/

// single line comments

// modules
let math = module("math");

import("all_extra.ph");

// values
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

let приклад = "phelt говорить вашою мовою";
let サンプル = "pheltはあなたの言語を話します";
let उदाहरण = "लक्स आपकी भाषा बोलता है";

// functions
fun add(a, b) {
  return a + b;
}

// inline, single expression, implicit return
fun sub(a, b) a - b;

// anonymous
let mul = fun(a, b) a * b;

// closures
fun make_adder(a) {
    fun adder(b) a + b;
    return adder;
}
let add1 = make_adder(1);
let add2 = make_adder(2);

// calling
let sum = add(1, 2);
let one = add1(0);

// operators
let add = 1 + 2; let sub = 1 - 2;
let mul = 1 * 2; let div = 1 / 2;
let mod = 1 % 2; let or = 1 | 2;
let xor = 1 ^ 2; let and = 1 & 2;
let not = !true;

let eq = 1 == 2; let lt = 1 < 2;
let gt = 1 > 2;  let ne = 1 != 2;
let le = 1 <= 2; let ge = 1 >= 2;

// classes
class Vector {
    init(x, y) {
        this.x = x;
        this.y = y;
    }

    // dunder methods, aka magic methods, support all operators
    // __add, __sub, __mul, __div, __mod, __or, __xor, __and
    // __eq, __lt, __gt, __not
    __add(other) Vector(this.x + other.x, this.y + other.y);
    __sub(other) Vector(this.x - other.x, this.y - other.y);
    __mul(other) Vector(this.x * other.x, this.y * other.y);
    __div(other) Vector(this.x / other.x, this.y / other.y);

    magnitude() math.sqrt(this.x * this.x + this.y * this.y);
    dot(other) this.x * other.x + this.y * other.y;
    cross(other) this.x * other.y - this.y * other.x;

    angle(other) math.atan2(this.cross(other), this.dot(other));
    scale(s) Vector(this.x * s, this.y * s);

    distance(other) (this - other).magnitude();
    normalize() this.scale(1 / this.magnitude());

    project(other) other.scale(this.dot(other) / other.dot(other));
    reflect(other) this - other.scale(2 * this.dot(other) / other.dot(other));
}

// inheritance
class Box < Vector {
    init(w, h, x, y) {
        super.init(x, y);
        this.w = w;
        this.h = h;
    }

    contains(other) {
        return other.x >= this.x && other.x <= this.x + this.w &&
               other.y >= this.y && other.y <= this.y + this.h;
    }
}

// instantiation
let b1 = Box(10, 10, 0, 0);
let b2 = Box(10, 10, 5, 5);

// calling
let dist = b1.distance(b2);
let touching = b1.contains(b2);

// control flow
if (true) {
    // true
} else if (false) {
    // false
} else {
    // neither
}

switch(1) {
    case 1:
        // one
    case 2:
        // two
    default:
        // default
}

// loops
let y = 0;
while (y++ < 10) {
    if (y % 2 == 0) {
        continue;
    }
}

for (let i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        break;
    }
}
