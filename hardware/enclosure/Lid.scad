// File: Enclosure_lid.scad
// Description: Enclosure lid for Dodepan
// Author: Turi Scandurra
// Website: https://www.turiscandurra.com/circuits
// Date: 2024-09-20
// License: MIT License

enclosure_r = 75;
tolerance = 1.0;
wall_w = 2.4;
height = 2.4;
mount_r = enclosure_r - 3.8;
enclosure_side_r = enclosure_r * 0.866;
inset_h = 0.6;
logo_w = 45;
logo_h = 48.8;
logo_inset = 0.6;
six_angles = [0, 60, 120, 180, 240, 300];

module graphics() {
    for (angle=six_angles) {
        // Ring 1
        rotate([0,0,angle])
                translate([22, 0, 0]) rotate([0,0,30]) cylinder(h=1, r=9, center=true, $fn=6);
        
        // Ring 2
        rotate([0,0,30+angle])
                translate([33, 0, 0]) cylinder(h=1, r=8, center=true, $fn=6);

        // Ring 3
        rotate([0,0,angle])
                translate([38, 0, 0]) rotate([0,0,30]) cylinder(h=1, r=6, center=true, $fn=6);
        
        // Ring 4 A
        rotate([0,0,18.5+angle])
                translate([44, 0, 0]) rotate([0,0,-48.5])cylinder(h=1, r=4, center=true, $fn=6);
            
        // Ring 4 B
        rotate([0,0,41.5+angle])
                translate([44, 0, 0]) rotate([0,0,-11.5])cylinder(h=1, r=4, center=true, $fn=6);
    }
}

module lid(){
    rotate([0,0,-30])
    difference(){
        // Main shape
        cylinder(r=enclosure_r-wall_w-tolerance/2, h=height, $fn=6);
        // Graphics
        translate([0,0,height]) rotate([0,0,30]) scale([1.3,1.3,inset_h]) graphics();
        // Logo
        rotate([0,0,30]) scale([0.6,0.6,1])
        translate([logo_w/-2, logo_h/-2, height - logo_inset]){
            linear_extrude(height = 1)
                import (file = "logo.dxf");
        }
        // Screw holes
        for (angle=six_angles) {
            rotate([0,0,angle])
                translate([mount_r-4,0,0]) 
                    cylinder(h=height, r1=1, r2=2, $fn=48);
        }
        // BOOTSEL
        rotate([0,0,30]) translate([-50.75, 9.5]) cylinder(h=height, r=1.66, $fn=64);
    }
}

// Render the part
lid();
