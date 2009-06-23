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

# setup plot
printf G ( "set terminal x11 font 'Helvetica,20'\n" );
printf G ( "set log y\n" );
printf G ( "unset xtics\n" );
printf G ( "unset ytics\n" );
printf G ( "set xlabel 'time'\n" );
printf G ( "set ylabel 'echo delta (log scale)'\n" );
printf G ( "set title 'Setting backlight level based on sliding window average sonar reading' font 'Helvetica,24'\n" );

while (<S>) { 
    if (/delta:(.*)\}/) {
	# parse out sonar reading
	$reading = $1;
	print F $reading, "\n";

	# calculate sliding window avg
	unshift( @queue, $reading ); #push
	$window_sum += $reading;
	$window_sum -= pop( @queue );
	$avg = $window_sum/$WINDOW_SIZE;
	print F2 $count," ",$avg,"\n";

	# SET BACKLIGHT
	# first, apply log scaling	
	$level = ( log($avg) - log($LOWEST) )/(log($HIGHEST)-log($LOWEST)); # \in [0,1)
	if( $level < 0 ){ $level = 0; }
	if( $level > 1 ){ $level = 1; }
	backlight( $level );

        # (RE)PLOT
	# set to fixed sliding x window
	printf G ( "set xrange [%d:%d]\n", $count-$MAXX,$count );

	printf G ( "plot " );
	
	# plot color gradient for backlight levels
	for( $i=$LCD_STEPS-1; $i>0; $i-- ){	    
	    $label='';
	    if( $i==1 ){ $label = 'backlight levels'; } 
	    $brightness_line = $LOWEST * (($HIGHEST/$LOWEST)**($i/$LCD_STEPS));
	    printf G ( "(%f) w filledcurves x1 fs solid %f ".
		       "lt rgb 'black' lw 0 title '%s',", 
		       $brightness_line, 0.5-0.5*$i/$LCD_STEPS, $label);
	}
	# plot two lines for current and average sonar reading
	printf G ( "'data.txt' w l lw 4 lt rgb 'blue' ".
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

