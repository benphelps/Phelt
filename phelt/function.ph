let foo = "fail";

fun bar() {
  let bar = foo;

  fun baz(bar) {
    bar = "pass";
    return bar;
  }

  return baz(bar);
}

println(bar());
