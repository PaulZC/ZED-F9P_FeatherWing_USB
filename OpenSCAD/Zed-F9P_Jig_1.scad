// Assembly jig to aid positioning of the ZED-F9P on the FeatherWing

$fn = 90;

// The jig is 2.0" * 0.9"
// Corners are centered on X = 0.1" and 1.9", Y = 0.1" and 0.8"

module base()
{
    union()
    {
            translate([0,0,0])cylinder(r=(0.1*25.4), h=0.5);
            translate([0,(0.7*25.4),0])cylinder(r=(0.1*25.4), h=0.5);
            translate([(1.8*25.4),0,0])cylinder(r=(0.1*25.4), h=0.5);
            translate([(1.8*25.4),(0.7*25.4),0])cylinder(r=(0.1*25.4), h=0.5);
    }
    translate([0,-(0.1*25.4),0])cube([(1.8*25.4),(0.9*25.4),0.5]);
    translate([-(0.1*25.4),0,0])cube([(2.0*25.4),(0.7*25.4),0.5]);
}

module spacers()
{
    union()
    {
        translate([0,0,0])cylinder(r=(0.075*25.4), h=1.0);
        translate([0,(0.7*25.4),0])cylinder(r=(0.075*25.4), h=1.0);
        translate([(1.8*25.4),0,0])cylinder(r=(0.075*25.4), h=1.0);
        translate([(1.8*25.4),(0.7*25.4),0])cylinder(r=(0.075*25.4), h=1.0);
    }
}

module pegs()
{
    union()
    {
        translate([0,0,0])cylinder(r=(0.045*25.4), h=1.75);
        translate([0,(0.7*25.4),0])cylinder(r=(0.045*25.4), h=1.75);
        translate([(1.8*25.4),0,0])cylinder(r=(0.045*25.4), h=1.75);
        translate([(1.8*25.4),(0.7*25.4),0])cylinder(r=(0.045*25.4), h=1.75);
    }
}

module wings()
{
    union()
    {
        translate([(0.9*25.4),(-0.075*25.4),0.5])cube([(1*25.4),(0.05*25.4),1.0],center=true);
        translate([(0.9*25.4),(0.775*25.4),0.5])cube([(1*25.4),(0.05*25.4),1.0],center=true);
    }
}

module hole()
{
    // Cube is taller than it needs to be to avoid zero thickness skins
    translate([32.38,11.43,0])cube([22.5,17.5,2.0],center=true);
}

module jig()
{
    difference()
    {
        translate ([(0.1*25.4),(0.1*25.4),0]) union()
        {
            base();
            spacers();
            pegs();
            wings();
        }
        hole();
    }
}

jig();
