CFLAGS= -std=gnu99 -Wall -Wextra -Wfatal-errors -pedantic -Wno-unused-parameter
INCLUDEFLAGES= -lncurses
PNAME=invaders
FETCHMEM=curl -L -O https://bintray.com/artifact/download/bruening/DrMemory/DrMemory-MacOS-1.8.1-0.tar.gz --progress-bar
H_FILE_DIR=../include
LIBTARGETDIR=../lib
STATICLIBFILENAME=libs
DYNLIBFILENAME=
LINE=\n-----------------------------------------------\n

all: clean server client

clean:
	rm -rf *o $(PNAME) $(PNAME).*

havedirs:
	@test -d $(H_FILE_DIR) || (mkdir $(H_FILE_DIR);echo "Yay, I proudly present to you, your first .h in your library! I put it in $(H_FILE_DIR) for you!\n")
	@test -d $(STATICLIBTARGETDIR) || (mkdir $(STATICLIBTARGETDIR);echo "Yay, I proudly present to you, your first product in your library! I put it in $(STATICLIBTARGETDIR) for you!\n")

run: all
	clear
	./prog

debug: all
	lldb ./$(PNAME)

gdebug: all
	lldb -s gui ./(PNAME)

getdrmem:
	@test -d ../../drmemory || (echo "\033[31m$(LINE)DRMemory seems to be missing!\nFetching NOW!\033[0m";$(FETCHMEM);echo "\033[31m$ Extracting package\033[0m";tar -zxf DrMemory-MacOS-1.8.1-0.tar.gz ;echo "\033[34m$(LINE)Deleting image!\033[0m";echo "\033[34m$(LINE)Moving extracted Folder!\033[0m";mv DrMemory-MacOS-1.8.1-0 drmemory;mv drmemory ../../;echo "\033[34mDONE$(LINE)\033[0m")

game:
	gcc -c game.c -I./include $(CFLAGS) $(INCLUDEFLAGES)
	gcc -v *.o  -o $(PNAME) -I./lib/*.a -I./lib/*.dylib $(CFLAGS) $(INCLUDEFLAGES)

server: clean
		gcc graphX.c server.c communication.c -o server $(CFLAGS) -lncurses

client: clean
	 gcc graphX.c client.c communication.c -o client $(CFLAGS) -lncurses -DNOSOUND

client_withsound: clean
	 gcc client.c  graphX.c communication.c -o client $(CFLAGS) -lncurses -DSOUND

audio:
	gcc audiodaemon.c -o adeamon -std=gnu99 $(CFLAGS) $(INCLUDEFLAGES)
	gcc audiosender.c -o asender -std=gnu99 $(CFLAGS) $(INCLUDEFLAGES)
