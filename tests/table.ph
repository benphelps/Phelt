let anon = fun() "haha";

let a = {
    bang: "da",
    1: "ok",
    true: "yep",
    false: "nope",
    [anon]: "hoho"
};

println(a[anon]);
