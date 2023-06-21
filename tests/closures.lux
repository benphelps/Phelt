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
