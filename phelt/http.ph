// tested using:
// docker run -p 80:80 kennethreitz/httpbin

let http = module("http");

dump(http);

println("--- GET ---");
let body = http.get("http://localhost/get");
println(body);

println("--- POST ---");
let body = http.post("http://localhost/post", "hello=world");
println(body);

println("--- PUT ---");
let body = http.put("http://localhost/put", "hello=world");
println(body);

println("--- DELETE ---");
let body = http.delete("http://localhost/delete", "");
println(body);

println("--- PATCH ---");
let body = http.patch("http://localhost/patch", "hello=world");
println(body);

println("--- HEAD ---");
let body = http.head("http://localhost/get");
println(body);

println("--- OPTIONS ---");
let body = http.options("http://localhost/get");
println(body);
