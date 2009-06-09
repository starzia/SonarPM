#!/usr/bin/perl -w
$HIGHEST=0.01;
$LOWEST=0.002;
$LCD_STEPS=8;
$MAXX=200;
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
	print G sprintf( "set terminal wxt 20\n" );
	print G sprintf( "set log y\n" );
	print G sprintf( "unset xtics\n" );
	print G sprintf( "unset ytics\n" );
	print G sprintf( "set xlabel 'time' font 'Helvetica,20'\n" );
	print G sprintf( "set ylabel 'echo delta' font 'Helvetica,20'\n" );
	print G sprintf( "set title 'Setting backlight level based on sliding window average sonar reading' font 'Helvetica,30'\n" );

	# set to fixed sliding x window
	print G sprintf( "set xrange [%d:%d]\n", $count-$MAXX,$count );

	print G sprintf( "plot " );
	
	# plot color gradient for backlight levels
	for( $i=$LCD_STEPS-1; $i>0; $i-- ){	    
	    $label='';
	    if( $i==1 ){ $label = 'backlight levels'; } 
	    $brightness_line = $LOWEST * (($HIGHEST/$LOWEST)**($i/$LCD_STEPS));
	    print G sprintf( "(%f) w filledcurves x1 fs solid %f ".
			     "lt rgb 'black' lw 0 title '%s',", 
			     $brightness_line, 0.5-0.5*$i/$LCD_STEPS, $label);
	}
	# plot two lines for current and average sonar reading
	print G sprintf( "'data.txt' w l lw 4 lt rgb 'blue' ".
			 "title '500 millisecond window', ".
			 "'data2.txt' w l lw 10 lt rgb 'red' ".
			 "title 'sliding window average, n=%d'\n",
			 $WINDOW_SIZE );
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
