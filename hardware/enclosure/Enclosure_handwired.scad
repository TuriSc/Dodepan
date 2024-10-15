// File: Enclosure_handwired.scad
// Description: Enclosure for Dodepan, handwired (no PCB) version
// Author: Turi Scandurra
// Website: https://www.turiscandurra.com/circuits
// Date: 2024-08-26
// License: MIT License

version_label = "v1.7";

enclosure_r = 75;
inner_h = 20.5;
tolerance = 1.0;

wall_w = 2.4;
floor_h = 2.4;
mount_w = 1.6;
mount_r = enclosure_r - 3.8;

electrode_hole_r = 0.75;
speaker_hole_r = 1.0;
screw_r = 1.0;

electrodes_r_maj = 36;
electrodes_r_min = 16;

snapfit_depth = 2;

pico_mount_height = 9 - floor_h;

six_angles=[0, 60, 120, 180, 240, 300];

// Computed values
enclosure_h = inner_h + floor_h;
enclosure_side_r = enclosure_r * 0.866;

function center(x,y,z) = [x/-2,y/-2,z/-2];

module lid_mount() {
    rotate(-90) translate([-1,-2,0]) cube([2,4,inner_h]);
    translate([-4,0,0]) standoff(inner_h);
}
  
module face_circles() {
    depth = 1;
    rotate_extrude($fn=128) {
        for (radius=[electrodes_r_min, (electrodes_r_min+electrodes_r_maj)/2, electrodes_r_maj]) {
            translate([radius, 0, 0]) square([electrode_hole_r*2,electrode_hole_r*depth], center=true);
        }
    }
}

module rounded_corners() {
    for (a=six_angles) {
            rotate([0,0,a]) translate([enclosure_r-4-0.600, 0, 0]) rotate([0,0,-30]) rotate_extrude(angle=60, $fn=64) {
            translate([4, 0, 0]) square([4,enclosure_h]);
        }
    }
}

module casing() {
    difference() {
        cylinder(r=enclosure_r, h=enclosure_h, $fn=6);
        translate ([0, 0, floor_h]) cylinder(r=(enclosure_r-wall_w), h=enclosure_h, $fn=6);
        rounded_corners();
    }
}

module lid_mounts() {
    for (angle=six_angles) {
        rotate([0,0,angle]){
            translate([mount_r,0,0]) lid_mount();
        }
    }
}

module snapfit_casings() {
    for (angle=[60, 120]) {
        rotate([0,0,30+angle]){
            translate([enclosure_side_r-snapfit_depth,0,inner_h+1]) cube([2,4,1.2], center=true);
        }
    }
}

module electrode_holes() {
        for ( i = [0 : 7] ){
            rotate([0,0,(45*i)])
            translate([0,electrodes_r_maj,0])
            cylinder(h=floor_h, r=electrode_hole_r, $fn=24);
        }
        for ( i = [0 : 3] ){
            rotate([0,0,(90*i)])
            translate([0,electrodes_r_min,0])
            cylinder(h=floor_h, r=electrode_hole_r, $fn=24);
        }
}

module speaker_holes() {
        cylinder(h=floor_h, r=speaker_hole_r, $fn=24);
        for ( i = [0 : 7] ){
            rotate([0,0,(45*i)])
            translate([0,8,0])
            cylinder(h=floor_h, r=speaker_hole_r, $fn=24);
        }
        for ( i = [0 : 3] ){
            rotate([0,0,(90*i)])
            translate([0,4,0])
            cylinder(h=floor_h, r=speaker_hole_r, $fn=24);
        }
}

module battery_holder() {
    max_object_height=inner_h-floor_h;
    translate([0,-13,0]) rotate(180) label("+    18650    -");
    difference() {
        union() {
            // Main shape
            translate(center(69+11,18+3.6,0)) cube([69+11,18+3.6,max_object_height]);
            // Supports
            translate(center(96,-4,0)) cube([96,2,max_object_height]);
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

module standoff(height) {
    difference() {
        cylinder(r = mount_w + screw_r, h = height,$fn=8);
        cylinder(r = screw_r, h = height,$fn=8);
    }
}
module label(string) {
    linear_extrude(height = 0.5)
    text(string, font = "Montserrat", halign="center", valign="center", size=3);
}

module GY_521() {
    translate([0,3,0]) label("GY-521");
    translate([8.15,9,0]) standoff(8);
    translate([7.15,-1,0]) cube([2,8,8]);
    translate([-7.85,9,0]) standoff(8);
    translate([-8.85,-1,0]) cube([2,8,8]);
}

module pico() {
    label("PICO");
    translate([-22.8,0,0]) {  // Top holes
        translate([0,-5.7,0]) standoff(pico_mount_height);
        translate([0,5.7,0]) standoff(pico_mount_height);
    }
    translate([24.2,0,0]) { // Bottom holes
        translate([0,-5.7,0]) standoff(pico_mount_height);
        translate([0,5.7,0]) standoff(pico_mount_height);
        translate([-2,-4,0]) cube([6,8,pico_mount_height]);
        translate([2,-4,0]) cube([2,8,pico_mount_height+1]);
    }   
}

module TP4056() {
    translate([0,-13,0]) label("TP4056");
    difference() {
        translate([-10,-8,0]) cube([20,4,8]);
        translate([-10,-8,5.4]) cube([19,4,8]);
    }
    difference() {
        translate([-10,-22,0]) cube([20,4,8]);
        translate([-9,-22,5.4]) cube([18,4,8]);
    }
    translate([-5.5,-36,0]) cube([11,4,8]);
    translate([-2,-26,0]) cube([4,4,5.4]);
    translate([-11,-12,0]) difference() {
        cylinder(r = mount_w + screw_r, h = 5.4);
        cylinder(r = screw_r, h = 5.4);
    }
}

module TP4056_holes() {
    rotate([90,0,0])
    linear_extrude(5) {
        hull() {
            translate([-3.6,0,0]) circle(1.8, $fn=64);
            translate([3.6,0,0]) circle(1.8, $fn=64);
        }
        translate([7.6,0,0]) circle(1.4, $fn=64); // LED hole
    }
}

module electrode_label(string) {
    linear_extrude(height = 0.5){
        text(string, font = "Montserrat", halign="center",valign="center",size=2.2);
        difference() {
            circle(2.4, $fn=16);
            circle(2, $fn=16);
        }
    }
}

module electrode_labels() {
    translate([0, -12]) electrode_label("0");
    translate([0, -32]) electrode_label("1");
    translate([28, -23]) electrode_label("2");
    translate([-29.5, -25]) electrode_label("3");
    translate([33, -2.5]) electrode_label("4");
    translate([-36, 4]) electrode_label("5");
    translate([28.3, 28.3]) electrode_label("6");
    translate([-28.3, 28.3]) electrode_label("7");
    translate([0, 40]) electrode_label("8");
    translate([19, -2.5]) electrode_label("9");
    translate([-19.5, 0]) electrode_label("10");
    translate([0, 20]) electrode_label("11");
}

module MAX98357() {
    amp_mount_h = 5;
    label("MAX98357");
    rotate([0,0,-90]) {
        translate([-6.3,7]) standoff(amp_mount_h);
        translate([-9.2,-5,0]) cube([2.2,9,amp_mount_h]);
        translate([-10.2,-5,0]) cube([1,9,amp_mount_h+1]);
        mirror([180,0,0]){
            translate([-6.3,7]) standoff(amp_mount_h);
            translate([-9.2,-5,0]) cube([2.2,9,amp_mount_h]);
            translate([-10.2,-5,0]) cube([1,9,amp_mount_h+1]);
        }
    }
}
    
module audio_socket() {
    audio_socket_mount_h = 5;
    translate([7,8,0]) label("JACK");
    translate([7,2.8,0]) standoff(audio_socket_mount_h);
    difference() {
        translate([-.2,-12,0]) cube([14.4,12,10]);
        translate([.8,-13,5]) cube([12.4,14.4,5]);
        translate([7.2,-10,7.5]) rotate([90,0,0]) cylinder(r=3.4,h=5,$fn=64);
    }
}

module audio_socket_hole() {
    translate([2.4,-20,5]) cube([9.4,12.4,5]);
    translate([7.2,-10,7.5]) rotate([90,0,0]) cylinder(r=3.4,h=5,$fn=64);
}

module speaker_mount() {
    speaker_mount_h=8;
    translate([0,-4]) label("SPEAKER");
    translate([-18.5,-9]) standoff(speaker_mount_h);
    translate([18.5,-9]) standoff(speaker_mount_h);
}

module OLED() {
    translate([-1.5,8]) label("OLED");
    oled_mount_top_h=5;
    translate([9,-9]) standoff(oled_mount_top_h);
    translate([9,9]) standoff(oled_mount_top_h);
    translate([-18,-9]) standoff(oled_mount_top_h);
    translate([-18,9]) standoff(oled_mount_top_h);
}

module MPR121() {
    rotate([0, 0, -90]) label("MPR121");
    mpr121_mount_h=5;
    translate([-16,-5,0]) cube([2,15,mpr121_mount_h]);
    translate([-18,-5,0]) cube([2,15,mpr121_mount_h+2]);
    difference() {
        translate([11,6,0]) cube([6,6,mpr121_mount_h+2]);
        translate([-15,-10,mpr121_mount_h]) cube([30,20,3]);
    }
    translate([-12,10.5]) standoff(mpr121_mount_h);
    translate([16,-4]) standoff(mpr121_mount_h);
}

module capacitor() {
    label("C1");
    cap_mount_h = 5;
    translate([-5,0,0]) standoff(cap_mount_h);
    translate([5,0,0]) standoff(cap_mount_h);
}

module enclosure() {
    // Casing, openings, holes, and negative parts
    rotate([0,0,-30]) {
        difference() {
            casing();
            snapfit_casings();
            // Rotary encoder
            rotate([0,0,60]) translate([-10,enclosure_side_r-14.5]) cylinder(r = 3+0.75, h = 5.4, $fn=32);
            // Power switch opening
            rotate([0,0,60]) translate([0,enclosure_side_r,inner_h/2+2]) cube([13+tolerance,5,8+tolerance], center=true);
            // Pico USB hole
            rotate([0,0,120]) translate([14.5,enclosure_side_r,inner_h/2+1.5]) cube([8.5,5,3], center=true);
            // Pico socket
            rotate([0,0,120]) translate([3,enclosure_side_r-3,pico_mount_height+2]) cube([22+tolerance,2,4.5]);
            // TP4056 socket and LED opening
            translate([-2,enclosure_side_r+2.4,11.6]) TP4056_holes();
            // Electrode, speaker holes and decorative circles
            face_circles();
            rotate([0,0,-60]) electrode_holes();
            rotate([0,0,30]) translate([0,-55,0]) speaker_holes();
            // OLED opening
            rotate([0,0,30]) translate([-16.6,44.75]) cube([31,12,5]);
            rotate([0,0,30]) translate([-22.7,44,1]) cube([39.5,13,5]);
            // Jack socket opening
            translate([-22,-enclosure_side_r+14, floor_h]) audio_socket_hole();
        } // End holes and negative parts
        lid_mounts();
        // Encoder reinforcement
        rotate(60) translate([-16, enclosure_side_r-6,0]) cube([4,4,inner_h]);
        // Version label
        translate([0,-enclosure_side_r+5, floor_h]) label(version_label);
        // Jack socket
        translate([-22,-enclosure_side_r+14, floor_h]) audio_socket();
    } // End rotate
    
    translate([0,0,floor_h]) {
        electrode_labels();
        // Rotary encoder
        translate([-44,37]) label("ENC.");
        // Gyroscope-accelerometer
        translate([0,-3]) GY_521();
        // 18650 battery
        translate([47,0]) rotate([0,0,90]) battery_holder();
        // Pico
        translate([-39, 14.5]) pico();
        // Speaker
        translate([0, -38]) speaker_mount();
        // Decoupling capacitor
        translate([-5, 30]) capacitor();
        
        // I2S DAC/Amplifier
        translate([25, 11]) MAX98357();
        // TP4056 battery charger
        rotate([0,0,-30]) translate([2,enclosure_side_r+2.4]) TP4056();
        // MPR121 capacitive touch sensor
        translate([-51,-13]) rotate([0, 0, 90]) MPR121();
        // 128x32 OLED
        translate([1,51]) OLED();
    }
}

// Finally, draw the top-level part
enclosure();
