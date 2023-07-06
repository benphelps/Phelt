let system = module("system");

class Zoo {
  init() {
    this.aarvark  = 1;
    this.baboon   = 1;
    this.cat      = 1;
    this.donkey   = 1;
    this.elephant = 1;
    this.fox      = 1;
  }
  ant()    this.aarvark;
  banana() this.baboon;
  tuna()   this.cat;
  hay()    this.donkey;
  grass()  this.elephant;
  mouse()  this.fox;
}

let zoo = Zoo();
let sum = 0;
let start = system.clock();
let batch = 0;
while (system.clock() - start < 10) {
  for (let i = 0; i < 10000; i = i + 1) {
    sum = sum + zoo.ant()
              + zoo.banana()
              + zoo.tuna()
              + zoo.hay()
              + zoo.grass()
              + zoo.mouse();
  }
  batch = batch + 1;
}

println(sum);
println(batch);
println(system.clock() - start);
