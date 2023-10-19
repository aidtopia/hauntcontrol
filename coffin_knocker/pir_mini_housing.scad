// Mini PIR sensor housing
// Adrian McCarthy

// Metric screw threads.
// https://en.wikipedia.org/wiki/ISO_metric_screw_thread
// * internal thread: tap=true and subtract the resulting shape
// * external thread: tap=false
module threads(h, d, pitch, tap=true, nozzle_d=0.4) {
    thread_h = pitch / (2*tan(30));
    // An M3 screw has a major diameter of 3 mm.  For a tap, we nudge it
    // up with the nozzle diameter to compensate for the problem of
    // printing accurate holes and to generally provide some clearance.
    d_major = d + (tap ? nozzle_d : -nozzle_d);
    d_minor = d_major - 2 * (5/8) * thread_h;
    d_max = d_major + 2*thread_h/8;
    d_min = d_minor - 2*thread_h/4;
    
    echo(str("M", d, "x", pitch, ": thread_h=", thread_h, "; d_major=", d_major, "; d_minor=", d_minor));

    x_major = 0;
    x_deep  = x_major + thread_h/8;
    x_minor = x_major - 5/8*thread_h;
    x_clear = x_minor - thread_h/4;
    y_major = pitch/16;
    y_minor = 3/8 * pitch;
    
    wedge_points = [
        [x_deep, 0],
        [x_minor, y_minor],
        [x_minor, pitch/2],
        [x_clear, pitch/2],
        [x_clear, -pitch/2],
        [x_minor, -pitch/2],
        [x_minor, -y_minor]
    ];

    r = d_major / 2;

    facets =
        ($fn > 0) ? max(3, $fn)
                  : max(5, ceil(min(360/$fa, 2*PI*r / $fs)));
    dtheta = 360 / facets;
    echo(str("dtheta for threads = ", dtheta));

    module wedge() {
        rotate([1.35, 0, 0])
            rotate([0, 0, -(dtheta+0.1)/2])
                rotate_extrude(angle=dtheta+0.1, convexity=10)
                    translate([r, 0])
                        polygon(wedge_points);
    }

    intersection() {
        union() {
            for (theta = [-180 : dtheta : h*360/pitch + 180]) {
                rotate([0, 0, theta]) translate([0, 0, pitch*theta/360])
                    wedge();
            }
            
            cylinder(h=h, d=(tap ? d_minor : d_min+nozzle_d));
        }
        cylinder(h=h, d=(tap ? d_max : d_major));
    }
}


module PIR_housing(nozzle_d=0.4) {
    th = 2;
    lens_d = 12.5;
    lip_d  = 14.2;
    lip_th = 0.4;
    neck_d = 10.4;
    cable_d = 5;
    thread_d = 18;
    thread_pitch = 1.5;
    thread_l = 4*thread_pitch;
    body_l = 28.5+th;
    body_d = thread_d+2*th;
    ziptie_w  = 5 + nozzle_d;
    ziptie_th = 1.5 + nozzle_d;
    ziptie_dy = (thread_d-ziptie_th)/2-nozzle_d;
    ziptie_dz = (body_l-thread_l)/2;

    $fs=nozzle_d/2;

    module body() {
        difference() {
            union() {
                cylinder(h=body_l-thread_l, d=body_d, $fn=6);
                translate([0, 0, body_l-thread_l])
                    threads(thread_l, thread_d, thread_pitch, tap=false,
                            nozzle_d=nozzle_d);
            }
            translate([0, 0, -1])
                cylinder(h=body_l+2, d=cable_d+nozzle_d);
            translate([0, 0, th])
                cylinder(h=body_l, d=neck_d+nozzle_d);
            translate([0, 0, body_l-lip_th]) {
                cylinder(h=lip_th+1, d=lip_d+nozzle_d);
            }
            
            translate([0, ziptie_dy, ziptie_dz]) {
                rotate([90, 0, 90]) {
                    linear_extrude(body_d, center=true) {
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
        }
    }
    
    module cap(cap_h=thread_l+th) {
        difference() {
            cylinder(h=cap_h, d=body_d, $fn=6);
            translate([0, 0, cap_h-thread_l+0.1])
                threads(thread_l, thread_d, thread_pitch, tap=true,
                        nozzle_d=nozzle_d);
            translate([0, 0, -1])
                cylinder(h=cap_h+1, d=lens_d+nozzle_d);
        }
    }

    spacing = body_d + 1;
    translate([0, spacing, 0]) body();
    translate([-spacing, 0, 0]) cap(thread_l+2*th);
    translate([0, 0, 0])        cap(thread_l+th);
    translate([ spacing, 0, 0]) cap(thread_l+3*th);
}

PIR_housing();
