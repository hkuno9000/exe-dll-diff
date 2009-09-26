# Makefile - for exediff
# $Id: Makefile,v 1.2 2004/06/30 06:59:44 hkuno Exp $

#-------------------------------------------------------------------------
# MAIN TARGET
all:	build

#-------------------------------------------------------------------------
# MACROS
#
TARGETS=exediff.exe

#.........................................................................
# common macro
!ifndef RM
#RM	=rm
RM	=del
!endif

#.........................................................................
# cl options
#	-W3	set warning level 3
#	-GX	enable C++ EH (same as /EHsc)
#	-GR	enable C++ RTTI
#	-GF	enable read-only string pooling
#	-Gy	separate functions for linker
#	-I"xx"	set system include path
#	-D"xx"	define macro xx=1
#	 -D"_UNICODE"	switch runtime for Unicode (TCHAR functions)
#	 -D"UNICODE"	switch Win32API for Unicode (xxxA -> xxxW)
#	-MD	link with MSVCRT.LIB
#	-MDd	link with MSVCRTD.LIB debug lib
#	-O1	minimize space optimizations
#	-Od	disable optimizations (default)
#	-Zi	enable debugging information
#	-ZI	enable Edit and Continue debug info
#	-Zs	syntax check only
#	-GZ	enable runtime debug checks (fill stack by 0xCC)
#	-Fd"xx"	name .PDB file (PDB is a debug info)
#	-Fo"xx"	object file or path
#	-link setargv.obj	support wildcard matching
#
!ifndef CPPFLAGS
CPPFLAGS =-nologo -W3 -GX -GR -GF -Gy -MD -O1 -D"WIN32" $(USEROPT)
!endif

!ifdef DEBUG
CPPFLAGS =$(CPPFLAGS) -MDd -Od -ZI -GZ -D"_DEBUG"
!endif

.cpp.cod:
	$(CPP) $(CPPFLAGS) -Fc -c $<
.cpp.exe:
	$(CPP) $(CPPFLAGS) $<
#	$(CPP) $(CPPFLAGS) $< -link setargv.obj

#-------------------------------------------------------------------------
# COMMANDS
#
cleanall: clean
	$(RM) *.ex? *.lib *.dll
clean:
	$(RM) *.obj *.obd *.cod *.res \
		*.bak *.tmp $$* \
		*.ncb *.opt *.plg \
		*.ilk *.idb *.pdb *.sbr *.bsc *.aps

rebuild: cleanall build

build: $(TARGETS)

man: html\exediff-manual.html

zip:
	cvs -n update
	zip source.zip Makefile Doxyfile *.cpp *.h
	zip exdiff.zip -j $(TARGETS) source.zip html\exediff-manual.html html\*.css

rel: rebuild gendoc man zip

#.........................................................................
# gen target
exediff.exe: exediff.cpp Makefile

#.........................................................................
# gen document
gendoc: html\index.html

viewdoc: html\index.html
	start $**

html\index.html: *.cpp Doxyfile usage.tmp example.tmp
	doxygen

usage.tmp: $(TARGETS)
	-exediff -h 2>$@

example.tmp: $(TARGETS)
	-echo exediff $(WINDIR)\system32\msvcp50.dll $(WINDIR)\system32\msvcp60.dll >$@
	-exediff      $(WINDIR)\system32\msvcp50.dll $(WINDIR)\system32\msvcp60.dll >>$@

html\exediff-manual.html: gendoc
	perl -n << html\main.html >$@
		next if /<div class=.qindex/;			# navi-bar ÇèúãéÇ∑ÇÈ.
		s/<img src=.doxygen.png.+border=0 >/doxygen/;	# footer ÇÃ doxygenÉçÉSÇtextÇ…íuÇ´ä∑Ç¶ÇÈ.
		print;
<<

#.........................................................................
# validation test
testrun: sorry_none.test

#end
