#!/usr/bin/perl -w
$HIGHEST=0.01;
$LOWEST=0.0;
$LCD_STEPS=8;
$MAXX=50;
$WINDOW_SIZE=5;

use FileHandle;

open(S,"./sonar --poll |");
S->autoflush(1);
open(F,">data.txt");
F->autoflush(1);
open(F2,">data2.txt");
F2->autoflush(1);
open(G,"|gnuplot");
G->autoflush(1);

$count=0;
$window_sum=0;
while (<S>) { 
    if (/delta:(.*)\}/) {
	$window_sum += $1;
	if( $count % $WINDOW_SIZE == 0 ){
	    $avg = $window_sum/$WINDOW_SIZE;
	    print F2 $count," ",$avg,"\n";
	    backlight( $avg );
	    $window_sum=0
	}
	print F $1, "\n";
	print G sprintf( "set xrange [%d:%d]\n", $count-$MAXX,$count );
	print G sprintf( "plot 'data.txt' with linespoints lw 5, 'data2.txt' w linespoints lw 5, (%f) w l lw 3, (%f) w l lw 3\n", $LOWEST, $HIGHEST);
	$count++;
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
