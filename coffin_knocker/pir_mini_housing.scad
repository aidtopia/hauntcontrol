// Mini PIR sensor housing
// Adrian McCarthy 2023

use <aidthread.scad>

module PIR_housing(nozzle_d=0.4) {
    th = 2;
    lens_d = 12.5;
    lip_d  = 14.2;
    lip_th = 0.4;
    neck_d = 10.4;
    neck_l = 3.8;
    neck_flat = neck_d - 3;
    cable_d = 5;
    cap_thread_d = 18;
    cap_thread_pitch = 1.5;
    cap_thread_l = 4*cap_thread_pitch;
    pg7_thread_d = 12;
    pg7_thread_pitch = 1.5;
    pg7_thread_l = 4.5*pg7_thread_pitch;
    sensor_l = 24;
    shell_h = pg7_thread_l + sensor_l + th;
    shell_d = max(cap_thread_d, pg7_thread_d) + 2*(th + nozzle_d);
    body_h = shell_h + cap_thread_l;
    reducer_d1 = pg7_thread_d + nozzle_d;
    reducer_d2 = neck_flat + nozzle_d;
    reducer_h = reducer_d1 - reducer_d2;  // 45 deg angle
    ziptie_w  = 5 + nozzle_d;
    ziptie_th = 1.5 + nozzle_d;
    ziptie_dy = (cap_thread_d - ziptie_th)/2 - nozzle_d;
    ziptie_dz = sensor_l/2 + cap_thread_l;
    zip_r = sensor_l/1.8;
    cap_l = cap_thread_l + th;  // minimum cap length
    cap_dl = 3;  // step size for increasing lengths of the cap

    $fs=nozzle_d/2;

    module body() {
        difference() {
            union() {
                // hexagonal shell
                cylinder(h=shell_h, d=shell_d, $fn=6);
                // threaded top
                translate([0, 0, shell_h])
                    AT_threads(cap_thread_l, cap_thread_d, cap_thread_pitch, tap=false,
                            nozzle_d=nozzle_d);
            }
            // threads for PG7 gland
            translate([0, 0, -0.01])
                AT_threads(pg7_thread_l+0.01, pg7_thread_d, pg7_thread_pitch, tap=true,
                    nozzle_d=nozzle_d);
            // reducer (cone-shaped to enable printing w/o supports)
            translate([0, 0, pg7_thread_l-0.01])
                cylinder(h=reducer_h, d1=reducer_d1, d2=reducer_d2, $fn=6);
            // cavity for board (and support beneath the neck)
            translate([0, 0, th]) {
                linear_extrude(body_h+1, convexity=4) {
                    intersection() {
                        circle(d=neck_d + nozzle_d);
                        square([neck_d+nozzle_d, neck_d-3], center=true);
                    }
                }
            }
            // cavity for the neck
            translate([0, 0, body_h-lip_th-neck_l])
                cylinder(h=neck_l+1, d=neck_d+nozzle_d);
            // recess for the lip of the lens
            translate([0, 0, body_h-lip_th])
                cylinder(h=lip_th+1, d=lip_d+nozzle_d);
            
            // Slot for attaching with a perpendicular zip tie.
            translate([0, ziptie_dy, ziptie_dz]) {
                rotate([90, 0, 90]) {
                    linear_extrude(shell_d, center=true) {
                        translate([-ziptie_th/2,-ziptie_w/2]) {
                            polygon([
                                [0, 0],
                                [ziptie_th, 0],
                                [ziptie_th, ziptie_w],
                                [0, ziptie_w+ziptie_th]
                            ]);
                        }
                    }
                }
            }
            
            // Slot for attaching with a parallel zip tie.
            translate([0, -shell_d+ziptie_th, ziptie_dz]) {
                rotate([0, 90, 0]) {
                    rotate_extrude(angle=180, convexity=4, $fa=3) {
                        translate([zip_r, 0]) {
                            square([ziptie_th, ziptie_w+nozzle_d], center=true);
                        }
                    }
                }
            }
        }
    }
    
    module cap(cap_h) {
        difference() {
            cylinder(h=cap_h, d=shell_d, $fn=6);
            translate([0, 0, cap_h-cap_thread_l+0.1])
                AT_threads(cap_thread_l, cap_thread_d, cap_thread_pitch, tap=true,
                        nozzle_d=nozzle_d);
            translate([0, 0, -1])
                cylinder(h=cap_h+1, d=lens_d+nozzle_d);
        }
    }
    
    module clamp_nut(clamp_h=thread_l+th) {
        difference() {
            union() {
                cylinder(h=th, d=shell_d, $fn=6);
                translate([0, 0, th-0.01])
                    threads(thread_l, thread_d, thread_pitch, tap=false,
                             nozzle_d=nozzle_d);
            }
            // cable entry
            translate([0, 0, -0.01])
                cylinder(h=clamp_nut_h+0.02, d1=clamp_nut_d1, d2=clamp_nut_d2);
        }
    }
    
    module radial_translate(angle, distance=1) {
        translate([distance*cos(angle), distance*sin(angle), 0]) {
            children();
        }
    }

    body();
    spacing = shell_d + 1;
//    radial_translate( 30, spacing) clamp_nut();
//    radial_translate( 90, spacing) cap(cap_l);
//    radial_translate(150, spacing) cap(cap_l+cap_dl);
    radial_translate(210, spacing) cap(cap_l+2*cap_dl);
}

PIR_housing();
