let file = module("file");

file.fputs(file.stdout, "What is your name: ");
let name = file.fgets(file.stdin, 100);
file.fputs(file.stdout, "Hello, " + name);
