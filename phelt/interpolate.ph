let a = "a";
let b = "b";
let c = "c";
let t = "a = {} b = {} c = {}" % (a, b, "{}" % (c));
println(t);
