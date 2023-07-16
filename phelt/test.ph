let a = 1;
let b = 5;


for (let i = a * a; i < b * b * b * b; i = i++) {
    for (let j = a * a; j < b * b * b * b; j = j++) {
        println("{}, {}", i * i, j * j);
    }
}
