let math = module("math");

// classes
class Vector {
    init(x, y) {
        this.x = x;
        this.y = y;
    }

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

    center() Vector(this.x + this.w / 2, this.y + this.h / 2);

    contains(other) {
        return other.x >= this.x && other.x <= this.x + this.w &&
               other.y >= this.y && other.y <= this.y + this.h;
    }
}

let b1 = Box(10, 10, 0, 0);
let b2 = Box(10, 10, 5, 5);
let dist = b1.distance(b2);
let touching = b1.contains(b2);

println("Distance: {}", dist);
println("Touching: {}", touching);

println(tostring(b1));
