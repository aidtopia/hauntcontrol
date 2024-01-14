// PIR bake-off bracket
// Adrian McCarthy 2021

use <aidbolt.scad>
use <aidutil.scad>

sensors = [
    ["Adafruit 189",    23,     "M2.5",     28.5/2,     0],
    ["Adafruit 4871",   12.3,   "",         0,          0],
    ["Generic",         23.2,   "M2",       29/2,       0],
    ["Parallax",        23,     "M2.5",     35/2,       0],
    ["SparkFun",        22,     "M2.5",     23/2,       27.5/2]
];


nozzle_d = 0.4;
thickness = 2.4;
spacing = 40;
led_d = 5;

difference() {
    union() {
        cube([spacing*len(sensors), spacing+10, thickness]);
        for (i=[0:len(sensors)-1]) translate([i*spacing + spacing/2, spacing/2, 0]) {
            sensor = sensors[i];
            translate([-spacing/2 + led_d + 1.5, spacing/2 - led_d/2, 0]) {
                linear_extrude(1.5*thickness, convexity=10)
                    scale([0.42, 0.45]) text(sensor[0], font="Arial");
            }
        }
    }

    for (i=[0:len(sensors)-1]) {
        sensor = sensors[i];
        translate([i*spacing + spacing/2, spacing/2, 0]) {
            // Opening for PIR's lens:
            lens_d = round_up(sensor[1], nozzle_d);
            translate([0, 0, -1])
                cylinder(d=lens_d, h=thickness+2, $fs=nozzle_d/2);

            screw  = sensor[2];
            screw_l = thickness+2;
            lr     = sensor[3];
            ud     = sensor[4];
            if (screw != "") translate([0, 0, thickness]) {
                if (lr != 0) {
                    translate([-lr, -ud]) bolt_hole(screw, screw_l);
                    translate([ lr, -ud]) bolt_hole(screw, screw_l);
                }
                if (ud != 0) {
                    translate([-lr,  ud]) bolt_hole(screw, screw_l);
                    translate([ lr,  ud]) bolt_hole(screw, screw_l);
                }
            }
            translate([-spacing/2 + led_d/2 + 1, spacing/2, 0])
                translate([0, 0, -1]) cylinder(d=led_d, h=thickness+2, $fs=nozzle_d/2);
        }
    }
}

