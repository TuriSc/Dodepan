// File: OLED bracket.scad
// Description: A bracket to hold the OLED in position. Only needed for the handwired version of Dodepan.
// Author: Turi Scandurra
// Website: https://www.turiscandurra.com/circuits
// Date: 2024-09-20
// License: MIT License

mount_w = 1.6;
screw_r = 1.0;
bracket_height = 4;
radius = mount_w + screw_r;

module standoff(height){
    difference() {
         cylinder(r = mount_w + screw_r, h = height,$fn=8);
         cylinder(r = screw_r, h = height,$fn=8);
    }
}

module OLED_bracket(){
    difference() {
        hull(){
            translate([0,-9]) standoff(1);
            translate([0,9]) standoff(1);
        }
        translate([0,-9])
            cylinder(r = screw_r, h = bracket_height,$fn=8);
        translate([0,9])
            cylinder(r = screw_r, h = bracket_height,$fn=8);
    }
    translate([radius*-1, radius*-2,1])
        cube([radius*2, radius*4, bracket_height]);
}

OLED_bracket();