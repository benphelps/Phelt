let bar = "bar";

switch (bar) {
    case "foo":
        println("fail");
    case "bar":
        println("pass");
    case "baz":
        println("fail");
}

switch ("foo") {
    default:
        println("pass");
}

switch ("bar") {
    case bar:
        println("pass");
}
