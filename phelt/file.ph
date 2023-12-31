let file = module("file");

let fp = file.fopen("test.txt", "w+");

// write
file.fwrite(fp, "Hello World");

// tell
let pos = file.ftell(fp);
println("pos: {}", pos);

// seek
file.fseek(fp, 0, file.SEEK_SET);

// read
let str = file.fread(fp, 5);
println(str);

// close
file.fclose(fp);

// remove
file.remove("test.txt");

// user input
let input = file.fgets(file.stdin, 128);

println("input: {}", input);
