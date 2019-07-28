#!/usr/bin/perl

use strict;
use warnings;

while(<>)
{
	s{(path=").*/((?:fritzing-parts|partfactory|Fritzing/parts/(?:user|contrib))/)}{$1$2}g;
	print $_;
}
