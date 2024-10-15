// File: Encoder knob.scad
// Description: Knob for an EC11 rotary encoder with a 6mm D-shaft
// Author: Turi Scandurra
// Website: https://www.turiscandurra.com/circuits
// Date: 2024-09-20
// License: MIT License

roof_height = 1;
tolerance = 0.4;
shaft_r = 3 + tolerance;

module knob(height, inset, radius, sides) {
    difference() {
        cylinder(r=radius,h=height,$fn=sides);
        difference() {
            cylinder(r=shaft_r,h=height-roof_height,$fa=0.01,$fs=0.25);
            translate([1.5,-4,0])
                cube([4,8,height-2]);
        }
        cylinder(r1=5.6,r2=5.0,h=inset,$fa=0.01,$fs=0.25);
    }
}

translate([0,0,8])
    rotate([180,0,0])
        knob(9, 3, 7, 6);