// File: Enclosure_PCB.scad
// Description: Enclosure for Dodepan, PCB version (THT)
// Author: Turi Scandurra
// Website: https://www.turiscandurra.com/circuits
// Date: 2024-09-20
// License: MIT License

version_label = "v2.5";

enclosure_r = 75;
inner_h = 20.5;
tolerance = 1.0;

wall_w = 2.4;
floor_h = 2.4;
mount_w = 1.6;
mount_r = enclosure_r - 3.8;

electrode_hole_r = 1.5;
speaker_hole_r = 1.0;
screw_r = 1.0;

mount_h = 3.5;
pcb_h = 1.6;

electrodes_r_maj = 36;
electrodes_r_min = 16;

pico_mount_height = 9 - floor_h;

// Computed values
enclosure_h = inner_h + floor_h;
enclosure_side_r = enclosure_r * 0.866;
pcb_plane = floor_h + mount_h + pcb_h;

// Definitions
six_angles=[0, 60, 120, 180, 240, 300];

function center(x,y,z) = [x/-2,y/-2,z/-2];

module lid_mount() {
    rotate(-90) translate([-1,-2,0]) cube([2,4,inner_h]);
    translate([-4,0,0]) standoff(inner_h);
}
  
module face_circles(){
    depth = 1;
    rotate_extrude($fn=128) {
        for (radius=[electrodes_r_min, (electrodes_r_min+electrodes_r_maj)/2, electrodes_r_maj]) {
            translate([radius, 0, 0])
                square([electrode_hole_r*2, electrode_hole_r*depth], center=true);
        }
    }
     
}

module rounded_corners() {
     for (a=six_angles) {
             rotate([0,0,a])
                translate([enclosure_r-4-0.600, 0, 0])
                    rotate([0,0,-30])
                        rotate_extrude(angle=60, $fn=64) {
                            translate([4, 0, 0]) square([4,enclosure_h]);
                        }
     }
}

module main_hexagon(){
    cylinder(r=enclosure_r, h=enclosure_h, $fn=6);
}

module inner_hexagon(){
    cylinder(r=(enclosure_r-wall_w), h=enclosure_h, $fn=6);
}

module casing(){
    difference() {
        main_hexagon();
        translate ([0, 0, floor_h]) inner_hexagon();
        rounded_corners();
    }
}

module lid_mounts(){
    for (angle=six_angles) {
        rotate([0,0,angle]){
            translate([mount_r,0,0]) {
                lid_mount();
            }
        }
    }
}

module electrode_holes(){
    for ( i = [0 : 7] ){
        rotate([0,0,(45*i)]){
            translate([0,electrodes_r_maj,0]) {
                cylinder(h=floor_h, r=electrode_hole_r, $fn=24);
            }
        }
    }
    for ( i = [0 : 3] ){
        rotate([0,0,(90*i)]){
            translate([0,electrodes_r_min,0]) {
                cylinder(h=floor_h, r=electrode_hole_r, $fn=24);
            }
        }
    }
}

module speaker_holes(){
    cylinder(h=floor_h, r=speaker_hole_r, $fn=24);
    for ( i = [0 : 7] ){
        rotate([0,0,(45*i)]){
            translate([0,8,0]) {
                cylinder(h=floor_h, r=speaker_hole_r, $fn=24);
            }
        }
    }
    for ( i = [0 : 3] ){
        rotate([0,0,(90*i)]){
            translate([0,4,0]) {
                cylinder(h=floor_h, r=speaker_hole_r, $fn=24);
            }
        }
    }
}


module battery_holder() {
    max_object_height=inner_h-floor_h;
    rotate(180) label("+    18650    ­-");
    difference(){
        union(){
            // Main shape
            translate(center(69+11,18+3.6,0))
                cube([69+11,18+3.6,max_object_height]);
            // Supports
            translate(center(96,-4,0))
                cube([98,2,max_object_height]);
        }
    // Battery socket
    translate(center(65+2,18+1,0)) cube([65+2,18+1,max_object_height]);
    // Positive terminal socket
    translate([35.5,0,0]) translate(center(1.5,24,0)) cube([1.5,24,max_object_height]);
    // Negative terminal socket
    translate([-35.5,0,0]) translate(center(1.5,24,0)) cube([1.5,24,max_object_height]);
    // Terminal openings
    translate(center(70,10,0)) cube([70,10,max_object_height]);
    // Circular cut
    translate([0,20,35]) rotate([90,0,0]) cylinder(h=55, r=36, $fn=128);
    }
        
}

module standoff(height){
    difference() {
         cylinder(r = mount_w + screw_r, h = height,$fn=64);
         cylinder(r = screw_r, h = height,$fn=64);
    }
}

module label(string){    
    linear_extrude(height = 0.6)
        text(string, font = "Montserrat Semibold", halign="center",valign="center", size = 3);
}

module TP4056_holes(){
    rotate([90,0,0])
    linear_extrude(5) {
        hull(){
            translate([-3.6,0,0]) circle(1.8, $fn=64);
            translate([3.6,0,0]) circle(1.8, $fn=64);
        }
        translate([7.6,0,0]) circle(1.4, $fn=64); // LED hole
    }
}

module audio_socket_hole() {
    translate([2.4,-20,0]) cube([9.4,12.4,5]);
    translate([7.2,-12,2.5]) rotate([90,0,0]) cylinder(r=3.6,h=5,$fn=64);
}

module pcb_mounts() {
    translate([-47, 33.2, 0]) standoff(mount_h); // 1
    translate([-4.4, +22.4, 0]) standoff(mount_h); // 2
    translate([33.5, 37, 0]) standoff(mount_h); // 3
    translate([-55.5,-5, 0]) standoff(mount_h); // 4
    translate([-4.5, -22, 0]) standoff(mount_h); // 5
}

module enclosure() {
    // Casing, openings, holes, and negative parts
    rotate([0,0,-30]) {
        difference() {
        casing();
        // Rotary encoder
        rotate([0,0,60]) translate([-10,enclosure_side_r-14.5]) union() {
            cylinder(r = 3+0.75, h = 5.4, $fn=32);
            translate([0,0,1.9]) rotate([0,0,-30]) cube([12.5,12.2,1], center=true);
        }
        // Power switch opening
        rotate([0,0,60]) translate([0,enclosure_side_r,inner_h/2+2])
            cube([13+tolerance,5,8+tolerance], center=true);
        // Pico USB hole
        rotate([0,0,120]) translate([13.5,enclosure_side_r, pcb_plane +2.4])
            cube([8.5,5,3], center=true);
        // TP4056 socket and LED opening
        rotate([0,0,120]) translate([-20.25,enclosure_side_r+2.4,pcb_plane + 3.0]) TP4056_holes();
        // Jack socket opening
        translate([-8.65,enclosure_side_r+14, pcb_plane]) audio_socket_hole();
        // Electrode, speaker holes and decorative circles
        face_circles();
        rotate([0,0,-60]) electrode_holes();
        rotate([0,0,30]) translate([0,-52,0]) speaker_holes();
        // OLED opening
         rotate([0,0,30]) translate([-17.5, 43.9, 0]) {
             // Opening
             translate([2,.5,0]) cube([31,12,5]);
             // Inset
             translate([-1,0,1]) cube([40.5,13,5]);
        }
    } // End holes and negative parts
    lid_mounts();
    // Version label
    translate([10,-enclosure_side_r+5, floor_h]) label(version_label);
    } // End rotate
    
    translate([0,0,floor_h]) {
        // 18650 battery
        intersection(){
            rotate([0,0,-30]) inner_hexagon();
            translate([47,-6,0]) rotate([0,0,79]) battery_holder();
        }
        // PCB Standoffs
        pcb_mounts();
    }
    
}

// Finally, draw the part
enclosure();
