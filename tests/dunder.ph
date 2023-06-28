class A {
    let a = 10;

    init(v) {
        this.v = v;
    }

    __add(other) this.v + other.v;
    __sub(other) this.v - other.v;
    __mul(other) this.v * other.v;
    __div(other) this.v / other.v;
    __gt(other) this.v > other.v;
    __lt(other) this.v < other.v;
    __eq(other) this.v == other.v;
    __and(other) this.v & other.v;
    __or(other) this.v | other.v;
    __xor(other) this.v ^ other.v;
    __not() !this.v;
}


let a = A(20);
let b = A(20);

a.a = 20;

println(a.a);
println(b.a);

let c = a == b;
println(c);
