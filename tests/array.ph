let array = module("array");

let a = [1, 5, 3, 4, 2];

array.push(a, 9);
let popped = array.pop(a);

array.insert(a, 0, 0);
let removed = array.remove(a, 0);

array.sort(a, fun(a, b) a > b);
array.reverse(a);

let firstIndex = array.find(a, 5);
let lastIndex = array.findLast(a, 5);
let mapped = array.map(a, fun(a) a * 2);
let filtered = array.filter(mapped, fun(a) a >= 5);
let reduced = array.reduce(filtered, fun(a, b) a + b, 0);
let tall = [1, 2, 3, [4, 5, 6, [7, 8, 9, [10, 11, 12]]]];
let flattened = array.flatten(tall);
