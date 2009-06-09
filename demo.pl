#!/usr/bin/perl -w
$HIGHEST=0.02;
$LOWEST=0.002;
$LCD_STEPS=8;
$MAXX=200;
$WINDOW_SIZE=10;

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

#init queue
@queue = qw();
for( $i=0; $i<$WINDOW_SIZE; $i++ ){
    unshift( @queue, $HIGHEST ); # push
}
$window_sum=$HIGHEST*$WINDOW_SIZE;

while (<S>) { 
    if (/delta:(.*)\}/) {
	$reading = $1;
	unshift( @queue, $reading ); #push
	$window_sum += $reading;
	$window_sum -= pop( @queue );
	$avg = $window_sum/$WINDOW_SIZE;
	print F2 $count," ",$avg,"\n";

	# apply log scaling	
	$level = ( log($avg) - log($LOWEST) )/(log($HIGHEST)-log($LOWEST)); # \in [0,1)
	if( $level < 0 ){ $level = 0; }
	if( $level > 1 ){ $level = 1; }
	backlight( $level );

	print F $reading, "\n";
	print G sprintf( "set log y\n" );
	print G sprintf( "set xrange [%d:%d]\n", $count-$MAXX,$count );
	print G sprintf( "plot 'data.txt' w linespoints lw 4 title 'echo delta', 'data2.txt' w l lw 10 title 'backlight setting (window average)', (%f) w l lw 3 title 'backlight highest', (%f) w l lw 3 title 'backlight lowest'\n", $HIGHEST, $LOWEST );
	$count++;
    }
}

# parameter brightness should be in range [0,1]
sub backlight{
    $level = $LCD_STEPS * $_[0];
    if ( $level > $LCD_STEPS-1 ){
	$level = $LCD_STEPS-1;
    }
    if ( $level < 0 ){
	$level = 0;
    }
    system( sprintf( "dbus-send --system --print-reply --dest=org.freedesktop.Hal /org/freedesktop/Hal/devices/dell_lcd_panel org.freedesktop.Hal.Device.LaptopPanel.SetBrightness int32:%d\n",$level ) );
}
