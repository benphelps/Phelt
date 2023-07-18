let json = module("json");

let input = '
{
    "name": "John", // JSON5, so comments are allowed
    "age": 30,
    "cars": [
        "Ford",
        "BMW",
        "Fiat"
    ],
}';

let obj = json.parse(input);
dump(obj);
dump(obj.cars);
