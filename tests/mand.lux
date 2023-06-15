let charmap = " .:-=+*#%@";

// size of the ASCII art
let width = 120;
let height = 40;

// range in the complex plane to visualize
let xmin = -2.1;
let xmax = 1.1;
let ymin = -1.3;
let ymax = 1.3;

// maximum number of iterations
let maxIteration = 100;

for(let y=0; y<height; y++) {
    for(let x=0; x<width; x++) {
        // scale x and y to the range we're visualizing
        let cx = (x / width) * (xmax - xmin) + xmin;
        let cy = (y / height) * (ymax - ymin) + ymin;

        let zx = 0;
        let zy = 0;
        let iteration = 0;

        while(zx*zx + zy*zy <= 4 and iteration < maxIteration) {
            let xtemp = zx*zx - zy*zy + cx;
            zy = 2*zx*zy + cy;
            zx = xtemp;
            iteration = iteration + 1;
        }

        if(iteration == maxIteration) {
            print(" ");
        } else {
            print(charmap[iteration % 10]);
        }
    }
    println("");
}
