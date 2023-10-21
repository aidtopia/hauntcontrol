// Coffin Knocker Mounts
// Adrian McCarthy 2023-10-11
//
// Electronics housing for replacement controller.

module rounded_rect(l, w, r) {
    dx = l/2 - r;
    dy = w/2 - r;
    hull () {
        translate([-dx,  dy]) circle(r);
        translate([-dx, -dy]) circle(r);
        translate([ dx,  dy]) circle(r);
        translate([ dx, -dy]) circle(r);
    }
}

module coffin_knocker_electronics_housing(nozzle_d=0.4) {
    th = 1.6;

    // Board is an Adafruit Perma-Proto half-sized breadboard
    // https://www.adafruit.com/product/571
    board_l = 82.0;
    board_w = 51.0;
    board_th = 1.6;
    board_corner_r = 3;
    board_descent = 6; // clearance needed below the board
    board_ascent = 15 + board_th;  // clearance needed above
    boss_dx = 74/2;
    boss_d = 3;
    // The FTDI connector on the Arduino Pro Mini hangs over the edge,
    // so we'll provide access and some protection against bent pins.
    ftdi_w = 16;
    ftdi_h = 6.5;
    ftdi_overhang = 7;
    ftdi_dx = (board_l + ftdi_overhang)/2;
    ftdi_dy = -3.5;
    ftdi_dz = 5.5;
    // An opening for the cable to the solenoid valve.
    valve_d = 5;
    valve_w = valve_d + 2*nozzle_d;
    valve_dx = (board_l + th)/2;
    valve_dy = -18.7;
    valve_dz = 5;
    // The other end of the board has a barrel jack for power.
    jack_w = 9;
    jack_h = 11;
    jack_dx = -(board_l + th)/2;
    jack_dy = -jack_w/2;
    // An opening for the cable to the motion sensor.
    sensor_d = 5;
    sensor_w = sensor_d + 2*nozzle_d;
    sensor_dx = (board_l + th)/2;
    sensor_dy = 18.7;
    sensor_dz = 5;
    // The base has "tabs" with holes for mounting with #6 flat head screws.
    tab_w = max(9, ftdi_overhang);
    screw_free_r  = (0.1495 * 25.4 + nozzle_d) / 2;
    screw_head_r  = (0.2620 * 25.4 + nozzle_d) / 2;
    screw_outer_r = screw_head_r + th;
    screw_sink    = 0.0830 * 25.4;
    screw_dx      = board_l/2 + screw_outer_r + 2*th + nozzle_d;
    screw_dy      = board_w/2 - screw_outer_r;
    // Now we can know the total footprint size of the base.
    base_l = board_l + 6*th + 2*nozzle_d + 2*tab_w;
    base_w = board_w + 2*th;
        
    module board_footprint() {
        rounded_rect(board_l, board_w, board_corner_r);
    }
    
    module board() {
        linear_extrude(board_th) {
            difference() {
                board_footprint();
                translate([-boss_dx, 0]) circle(d=boss_d);
                translate([ boss_dx, 0]) circle(d=boss_d);
            }
        }
    }
    
    module base_footprint() {
        rounded_rect(base_l, base_w, board_corner_r);
    }
    
    module support_wall_footprint() {
        difference() {
            board_footprint();
            offset(r=-th) board_footprint();
        }
    }
    
    module perimeter_wall_footprint() {
        difference() {
            offset(r=th) board_footprint();
            offset(r=nozzle_d/2) board_footprint();
        }
    }
    
    module boss_footprints() {
        translate([-boss_dx, 0]) circle(d=boss_d-nozzle_d);
        translate([ boss_dx, 0]) circle(d=boss_d-nozzle_d);
    }

    module jack_support() {
        linear_extrude(th+board_descent+board_th)
            translate([jack_dx, jack_dy])
                square([th, jack_w], center=true);
    }

    module ftdi_support() {
        linear_extrude(th+board_descent+ftdi_dz) {
            translate([ftdi_dx, ftdi_dy])
                square([ftdi_overhang, ftdi_w], center=true);
        }
    }
    
    module valve_support() {
        z = th+board_descent+valve_dz;
        difference() {
            linear_extrude(z) {
                translate([valve_dx, valve_dy])
                    square([th, valve_w], center=true);
            }
            translate([valve_dx, valve_dy, z])
                rotate([0, 90, 0])
                    cylinder(h=th+2, d=valve_d, center=true);
        }
    }
    
    module sensor_support() {
        z = th+board_descent+sensor_dz;
        difference() {
            linear_extrude(z) {
                translate([sensor_dx, sensor_dy])
                    square([th, sensor_w], center=true);
            }
            translate([sensor_dx, sensor_dy, z])
                rotate([0, 90, 0])
                    cylinder(h=th+2, d=sensor_d, center=true);
        }
    }
    
    module screw_support() {
        rotate_extrude(convexity=4)
            polygon([
                [screw_free_r, 0],
                [screw_free_r, th],
                [screw_head_r, th+screw_sink],
                [screw_outer_r, th+screw_sink],
                [screw_outer_r, 0]        
            ]);
    }
    
    module base() {
        difference() {
            union() {
                linear_extrude(th)
                    base_footprint();

                translate([-screw_dx,  screw_dy]) screw_support();
                translate([-screw_dx, -screw_dy]) screw_support();
                translate([ screw_dx,  screw_dy]) screw_support();
                translate([ screw_dx, -screw_dy]) screw_support();

                linear_extrude(th+board_descent)
                    support_wall_footprint();

                linear_extrude(th+board_descent+board_th) {
                    boss_footprints();
                }
                jack_support();
                ftdi_support();
                valve_support();
                sensor_support();
            }
            
            translate([0, 0, -1])
            linear_extrude(th + screw_sink + 2) {
                translate([-screw_dx,  screw_dy]) circle(r=screw_free_r);
                translate([-screw_dx, -screw_dy]) circle(r=screw_free_r);
                translate([ screw_dx,  screw_dy]) circle(r=screw_free_r);
                translate([ screw_dx, -screw_dy]) circle(r=screw_free_r);
            }
        }
    }
    
    module cover() {
        difference() {
            linear_extrude(board_descent+board_ascent, convexity=10)
                perimeter_wall_footprint();
            translate([0, 0, -1]) {
                linear_extrude(board_descent+board_th+jack_h+1) 
                    translate([jack_dx, jack_dy])
                        square([th+2, jack_w], center=true);
                linear_extrude(board_descent+ftdi_dz+ftdi_h+1)
                    translate([board_l/2+th/2, ftdi_dy])
                        square([th+2, ftdi_w], center=true);
                linear_extrude(board_descent+board_th+valve_dz+1)
                    translate([valve_dx, valve_dy])
                        square([th+2, valve_w], center=true);
                linear_extrude(board_descent+board_th+sensor_dz+1)
                    translate([sensor_dx, sensor_dy])
                        square([th+2, sensor_w], center=true);
            }
            translate([valve_dx, valve_dy, board_descent+board_th+valve_dz])
                rotate([0, 90, 0])
                    cylinder(h=2+th, d=valve_d, center=true);
            translate([sensor_dx, sensor_dy, board_descent+board_th+sensor_dz])
                rotate([0, 90, 0])
                    cylinder(h=2+th, d=sensor_d, center=true);
        }
        translate([0, 0, board_descent+board_ascent]) {
            difference() {
                linear_extrude(th)
                    offset(r=th) board_footprint();
                translate([0, 0, th/2])
                    linear_extrude(th, convexity=10) {
                        translate([0, 4])
                            text("Coffin Knocker", size=8.5, halign="center");
                        translate([0, -4])
                            text("Controller by Adrian McCarthy", size=4.2, halign="center");
                        translate([0, -10.2])
                            text("for Dragon Vane Cove", size=4.2, halign="center");
                        translate([0, -16.4])
                            text("October 2023", size=4.2, halign="center");
                    }
            }
        }
    }

    $fs = nozzle_d/2;
    base();
    translate([0, base_w + 2, board_descent+board_ascent+th])
        rotate([180, 0, 0])
            cover();
}

coffin_knocker_electronics_housing();
