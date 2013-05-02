# 
#  Copyright (c) 1997 by the University of Southern California
#  All rights reserved.
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation in source and binary forms for non-commercial purposes
#  and without fee is hereby granted, provided that the above copyright
#  notice appear in all copies and that both the copyright notice and
#  this permission notice appear in supporting documentation. and that
#  any documentation, advertising materials, and other materials related
#  to such distribution and use acknowledge that the software was
#  developed by the University of Southern California, Information
#  Sciences Institute.  The name of the University may not be used to
#  endorse or promote products derived from this software without
#  specific prior written permission.
# 
#  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
#  the suitability of this software for any purpose.  THIS SOFTWARE IS
#  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
#  Other copyrights might apply to parts of this software and are so
#  noted when applicable.
# 
#  $Header: /cvsroot/nsnam/nam-1/bin/gen-vcmake.pl,v 1.5 1998/09/02 21:23:25 haoboy Exp $
#
# This is not to be used as an executable. Rather, it's intended to be invoked
# from Makfefile to generate a makefile.vc

while (<>) {
    (/^\$\(GEN_DIR\)nam_tcl\.cc/ || /^\$\(GEN_DIR\)version.c/) && do {
	# print current line followed by a '-mkdir gen...'
	print $_;
	print "\t-mkdir \$(GEN_DIR:\\\\=)\n";
	next;
    };

    /^makefile\.vc:/ && do {
	# skip this line and the next two lines;
	<>; <>;
	next;
    };

    s/xwd\.o/win32.o getopt.o/o;
    s/^# (\!include)/\!include/o;
    s/\$\(\.SUFFIXES\)//o;
    s/OBJ\) Makefile/OBJ\) makefile.vc/o;
    print $_;
}

exit 0;

