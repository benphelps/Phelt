let string = module("string");

let str = "Hello World!";

dump(string.upper(str));
dump(string.lower(str));
dump(string.split(str, " "));
dump(string.trim("-----Hello World!----",  "-"));
dump(string.sub(str, 1, 5));
dump(string.find(str, "World"));
dump(string.replace(str, "Hello", "Goodbye"));
dump(string.reverse(str));
