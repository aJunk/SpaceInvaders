CFLAGS=-g -ansi -Wall -Wextra -Wfatal-errors -pedantic -pedantic-errors -m32
INCLUDEFLAGES= -lncurses
PNAME=invaders
FETCHMEM=curl -L -O https://bintray.com/artifact/download/bruening/DrMemory/DrMemory-MacOS-1.8.1-0.tar.gz --progress-bar
H_FILE_DIR=../include
LIBTARGETDIR=../lib
STATICLIBFILENAME=libs
DYNLIBFILENAME=
LINE=\n-----------------------------------------------\n

all: clean havedirs
	gcc -c *.c -I./include $(INCLUDEFLAGES)
	gcc -v *.o  -o $(PNAME) -I./lib/*.a -I./lib/*.dylib $(INCLUDEFLAGES)

clean:
	rm -rf *o $(PNAME) $(PNAME).*

havedirs:
	@test -d $(H_FILE_DIR) || (mkdir $(H_FILE_DIR);echo "Yay, I proudly present to you, your first .h in your library! I put it in $(H_FILE_DIR) for you!\n")
	@test -d $(STATICLIBTARGETDIR) || (mkdir $(STATICLIBTARGETDIR);echo "Yay, I proudly present to you, your first product in your library! I put it in $(STATICLIBTARGETDIR) for you!\n")

static: clean havedirs
	gcc -c $(STATICLIBFILENAME).c -I$(H_FILE_DIR) -m32
	ar rcs $(STATICLIBFILENAME).a $(STATICLIBFILENAME).o
	-rm $(LIBTARGETDIR)/$(STATICLIBFILENAME).a
	mv $(STATICLIBFILENAME).a $(LIBTARGETDIR)
	cp $(STATICLIBFILENAME).h $(H_FILE_DIR)/$(STATICLIBFILENAME).h

dynamic: clean havedirs
	gcc -v -dynamiclib -current_version 1.0  -o $(DYNLIBFILENAME).dylib -I../misc ../misc/*.a
	file $(DYNLIBFILENAME).dylib
	otool -L $(DYNLIBFILENAME).dylib
	-rm $(LIBTARGETDIR)/$(DYNLIBFILENAME).dylib
	mv $(DYNLIBFILENAME).dylib $(LIBTARGETDIR)
	cp $(DYNLIBFILENAME).h $(H_FILE_DIR)/$(DYNLIBFILENAME).h

run: all
	clear
	./prog

debug: all
	lldb ./$(PNAME)

gdebug: all
	lldb -s gui ./(PNAME)

getdrmem:
	@test -d ../../drmemory || (echo "\033[31m$(LINE)DRMemory seems to be missing!\nFetching NOW!\033[0m";$(FETCHMEM);echo "\033[31m$ Extracting package\033[0m";tar -zxf DrMemory-MacOS-1.8.1-0.tar.gz ;echo "\033[34m$(LINE)Deleting image!\033[0m";echo "\033[34m$(LINE)Moving extracted Folder!\033[0m";mv DrMemory-MacOS-1.8.1-0 drmemory;mv drmemory ../../;echo "\033[34mDONE$(LINE)\033[0m")