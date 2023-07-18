let json = module("json");

let input = '
{
    "name": "John", // JSON5, so comments are allowed
    "age": 30,
    "cool": true,
    "debt": null,
    "cars": [
        "Ford",
        "BMW",
        "Fiat"
    ],
}';

let obj = json.decode(input);
dump(obj);

let simplified = """
name = "John"
age = 30
cool = true
debt = null
cars = [
    "Ford"
    "BMW"
    "Fiat"
]
""";

let obj = json.decode(simplified, true);
dump(obj);
