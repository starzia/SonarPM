#!/usr/bin/perl -w
use FileHandle;

open(S,"./sonar_tui --poll |");
S->autoflush(1);
open(F,">data.txt");
F->autoflush(1);
open(G,"|gnuplot");
G->autoflush(1);

while (<S>) { 
    if (/delta:(.*)\}/) { 
	print F $1, "\n";
	print G "plot 'data.txt' with linespoints\n";
    }
}

