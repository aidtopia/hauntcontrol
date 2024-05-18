// Testing dimensions to panel-mount a Parallax brand thumbstick module.

m2_tap_d = 1.6;


module parallax_thumbstick_mount(nozzle_d=0.4) {
    $fs=nozzle_d/2;
    th = 2;
    pilot_d = m2_tap_d;
    pilot_l = 10;
    dx = 15/2;
    dy = 30.5/2;

    module mounts() {
        translate([-dx, -dy]) children();
        translate([ dx, -dy]) children();
        translate([-dx,  dy]) children();
        translate([ dx,  dy]) children();
    }

    module panel() {
        linear_extrude(th) {
            difference() {
                square([30, 36], center=true);
                offset(r=nozzle_d/2) circle(d=21);
            }
        }
    }

    module posts() {
        boss_d = 4;
        boss_h = max(19 - th, 17);
        difference() {
            union() {
                mounts() { cylinder(h=boss_h, d=boss_d); }
                linear_extrude(boss_h/3) {
                    translate([0, -dy]) square([2*dx, 5*nozzle_d], center=true);
                    translate([0,  dy]) square([2*dx, 5*nozzle_d], center=true);
                }
            }
            translate([0, 0, boss_h-pilot_l]) {
                mounts() cylinder(h=pilot_l+0.1, d=pilot_d+nozzle_d);
            }
            translate([0, 0, boss_h-0.6]) {
                mounts() cylinder(h=0.7, d1=pilot_d+nozzle_d, d2=pilot_d+2*nozzle_d);
            }
        }
    }

    panel();
    translate([0, 0, th]) posts();
}

parallax_thumbstick_mount();
