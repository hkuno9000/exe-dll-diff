# Makefile - for exediff
#
# Project Home: http://code.google.com/p/exe-dll-diff/
# Code license: New BSD License
#-------------------------------------------------------------------------
# MACROS
#
TARGET=Release\exediff.exe

#-------------------------------------------------------------------------
# MAIN TARGET
all:	build

#-------------------------------------------------------------------------
# COMMANDS
#
cleanall: clean
	-del *.zip *.ncb *.user *.dat *.cache *.bak *.tmp $$*
	-del src\*.aps src\*.bak
	-del html\*.html html\*.???

clean:
	vcbuild /clean   exediff.sln

rebuild:
	vcbuild /rebuild exediff.sln

build:
	vcbuild exediff.sln

zip:
	svn status
	zip exe-dll-diff-src.zip Makefile Doxyfile *.sln *.vcproj src/* test/* -x *.aps
	zip exe-dll-diff-exe.zip -j Release/*.exe html/*-manual.html html/*.css

rel: rebuild gendoc man zip
#.........................................................................
# DOCUMENT
#
man: html\exediff-manual.html

gendoc: html\index.html

viewdoc: html\index.html
	start $**

html\index.html: src/*.cpp Doxyfile usage.tmp example.tmp
	doxygen

usage.tmp: $(TARGET)
	-$(TARGET) -h 2>$@

example.tmp: $(TARGET)
	-echo exediff $(WINDIR)\system32\msvcp50.dll $(WINDIR)\system32\msvcp60.dll >$@
	-$(TARGET)    $(WINDIR)\system32\msvcp50.dll $(WINDIR)\system32\msvcp60.dll >>$@

html\exediff-manual.html: gendoc Makefile
	perl -n << html\main.html >$@
		next if /^<div class="navigation"/.../^<\/div>/;		# navi-bar ‚ğœ‹‚·‚é.
		s/<img class="footer" src="doxygen.png".+\/>/doxygen/;	# footer ‚Ì doxygenƒƒS‚ğtext‚É’u‚«Š·‚¦‚é.
		print;
<<

#.........................................................................
# TEST
#
testrun: sorry_none.test

# Makefile - end
