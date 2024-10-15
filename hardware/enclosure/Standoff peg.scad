// File: Standoff peg.scad
// Description: A small peg used to hold modules in position. Only needed for the handwired version of Dodepan.
// Author: Turi Scandurra
// Website: https://www.turiscandurra.com/circuits
// Date: 2024-09-20
// License: MIT License

mount_w = 1.6;
screw_r = 1.0;

module standoff_peg(height){
    peg_h = 1.5;
    radius = mount_w + screw_r;
    diam = radius * 2;
    difference() {
        union() {
            translate([diam*-1, radius*-1, 0])
                cube([diam*2, radius + screw_r, height+peg_h]);
            translate([-radius, 0]) cube([diam, diam, peg_h]);
        }
        cylinder(r = screw_r, h = height + peg_h,$fn=9);
      }
}

standoff_peg(1.5);