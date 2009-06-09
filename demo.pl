#!/usr/bin/perl -w
$HIGHEST=0.01;
$LOWEST=0.0;
$LCD_STEPS=8;
$MAXX=50;

use FileHandle;

open(S,"./sonar --poll |");
S->autoflush(1);
open(F,">data.txt");
F->autoflush(1);
open(G,"|gnuplot");
G->autoflush(1);

$count=0;
while (<S>) { 
    if (/delta:(.*)\}/) {
	$count++;
	backlight( $1 );
	print F $1, "\n";
	print G sprintf( "set xrange [%d:%d]\n", $count-$MAXX,$count );
	print G sprintf( "plot 'data.txt' with linespoints, (%f) w l, (%f) w l\n", $LOWEST, $HIGHEST);
    }
}

sub backlight{
    $level = ( ( $_[0] - $LOWEST )/$HIGHEST ) * $LCD_STEPS;
    if ( $level > $LCD_STEPS-1 ){
	$level = $LCD_STEPS-1;
    }
    if ( $level < 0 ){
	$level = 0;
    }
    system( sprintf( "dbus-send --system --print-reply --dest=org.freedesktop.Hal /org/freedesktop/Hal/devices/dell_lcd_panel org.freedesktop.Hal.Device.LaptopPanel.SetBrightness int32:%d\n",$level ) );
}
