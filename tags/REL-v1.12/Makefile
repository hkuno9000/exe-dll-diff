# Makefile - for exediff
#
# Project Home: http://code.google.com/p/exe-dll-diff/
# Code license: New BSD License
#-------------------------------------------------------------------------
# MACROS
#
TARGET=Release\exediff.exe
MANUAL=html\exediff-manual.html
DOXYINDEX=html\index.html

#-------------------------------------------------------------------------
# MAIN TARGET
all:	build

rel:	rebuild $(MANUAL) zip

#-------------------------------------------------------------------------
# COMMANDS
#
cleanall: clean
	-del *.zip *.ncb *.user *.dat *.cache *.bak *.tmp $$*
	-del src\*.aps src\*.bak
	-del /q html\*.* Release\*.* Debug\*.*

clean:
	vcbuild /clean   exediff.sln

rebuild:
	vcbuild /rebuild exediff.sln

build: $(TARGET)

$(TARGET): src/*.* Makefile
	vcbuild exediff.sln

zip:
	svn status
	zip exe-dll-diff-src.zip Makefile Doxyfile *.sln *.vcproj *.vsprops src/* test/* -x *.aps
	zip exe-dll-diff-exe.zip -j Release/*.exe html/*-manual.html html/*.css

install: $(TARGET) $(MANUAL)
	copy $(TARGET) \home\bin
	copy $(MANUAL)  docs
	copy html\*.css docs

man: $(MANUAL)
	start $**

doxy: $(DOXYINDEX)
	start $**

#.........................................................................
# DOCUMENT
#
$(DOXYINDEX): src/*.cpp Doxyfile usage.tmp example.tmp
	doxygen

usage.tmp: $(TARGET) Makefile
	-$(TARGET) -h 2>$@

example.tmp: $(TARGET) Makefile
	-echo exediff $(WINDIR)\system32\msvcp50.dll $(WINDIR)\system32\msvcp60.dll >$@
	-$(TARGET)    $(WINDIR)\system32\msvcp50.dll $(WINDIR)\system32\msvcp60.dll >>$@

$(MANUAL): $(DOXYINDEX) Makefile
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
