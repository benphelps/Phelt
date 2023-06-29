let table = module("table");

let anon = fun() "haha";

let a = {
    bang: "da",
    1: "ok",
    true: "yep",
    false: "nope",
    [anon]: "hoho"
};

dump(table.keys(a));
dump(table.values(a));
dump(a);
table.remove(a, "bang");
dump(a);
table.insert(a, "bang", "da");
dump(a);
dump(table.hasKey(a, "bang"));
dump(table.hasKey(a, "bong"));
