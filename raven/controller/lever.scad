nozzle_d = 0.4;


module lever() {
    linear_extrude(4) {
        difference() {
            hull() {
                circle(d=12);
                translate([0, 30]) circle(d=6);
            }
            circle(d=6);
        }
        translate([0, 30]) circle(d=8);
    }
}


module potentiometer_mount() {
    d = 20;
    l = 15;
    th = 2;
   
    module base_footprint() {
        translate([-d/2, -d/2-3]) square([d, th]);
    }
    
    module fillet() {
        polygon([
            [0, 0],
            [0, 3*th],
            [-th, 3*th],
            [-3*th, th],
            [-3*th, 0]
        ]);
    }
    
    linear_extrude(th) {
        difference() {
            hull() {
                circle(d=d);
                base_footprint();
            }
            circle(d=7);
            translate([-7.8, 0]) offset(r=nozzle_d/2) square([1.2, 2.8], center=true);
        }
    }
    linear_extrude(l) base_footprint();
    
    translate([-(d-th)/2, -d/2-3]) rotate([0, 90, 0]) linear_extrude(th, center=true) fillet();
    translate([(d-th)/2, -d/2-3]) rotate([0, 90, 0]) linear_extrude(th, center=true) fillet();
}

module trigger_assembly() {
    th = 2;
    fulcrum_d = 3;
    magnet_d = 6;
    magnet_th = 2.8;
    trigger_th = max(4, magnet_d+th);

    module base() {
        linear_extrude(th) translate([-25, -35]) offset(r=3) offset(r=-3) square([30, 40]);
        linear_extrude(th+trigger_th) circle(d=fulcrum_d);
    }

    module trigger() {
        difference() {
            linear_extrude(trigger_th) {
                difference() {
                    hull() {
                        offset(r=th) circle(d=fulcrum_d);
                        translate([0, -15]) offset(delta=th) square(fulcrum_d, center=true);
                        translate([15, -15]) circle(d=5);
                    }
                    offset(r=nozzle_d) circle(d=fulcrum_d);
                }
            }
            translate([-th+nozzle_d, -15, (trigger_th-magnet_d)/2]) {
                rotate([0, -90, 0]) {
                    linear_extrude(magnet_th+nozzle_d, center=true) {
                        offset(r=nozzle_d/2) hull() {
                            translate([magnet_d/2, 0]) circle(d=magnet_d);
                            translate([magnet_d, 0]) square([trigger_th, magnet_d], center=true);
                        }
                    }
                }
            }
        }
    }
    
    base();
    if ($preview) {
        translate([0, 0, th+0.1]) trigger();
    } else {
        translate([10, 0, 0]) trigger();
    }
}

translate([-20, 0]) lever($fs=nozzle_d/2);
potentiometer_mount($fs=nozzle_d/2);


!trigger_assembly($fs=nozzle_d/2);
