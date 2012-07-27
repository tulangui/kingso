#!/bin/perl
#use strict;
my $recline;
my $lineno = 0;

open Infile,"<$ARGV[0]"||die "can't open $ARGV[0]\n";
while(<Infile>)
{
     $recline=$_;
     chomp($recline);
     my @aa = split(//, $recline);
     my $rownum = ($aa[0]-1)/$aa[1]; 
     my $row;
     my $col;
     for ($row=1; $row<$rownum ; $row++)
     {
         for ($col=0; $col<$aa[1]; $col++)
         {
            print "$aa[$col+2] = $aa[$row*$aa[1]+$col+2]\n";  
        }
     }
}
