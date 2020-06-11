
#Creates the httpserver
#Jake Armendariz
#Makefile-- creates an executable: httpserver
SOURCES	 = server.c	requests.c	library.c	thread.c
OBJECTS	 = server.o	requests.o	library.o	thread.o
HEADERS	 = server.h
EXEBIN	 = httpserver
FLAGS	 = -Wall	-Wextra	-Wpedantic	-Wshadow	-g
#gdb --args httpserver 8080 -N 5 -l logfile

all: $(EXEBIN)

$(EXEBIN) : $(OBJECTS) $(HEADERS)
	gcc -o $(EXEBIN) $(OBJECTS)	-pthread

$(OBJECTS) : $(SOURCES) $(HEADERS)
	gcc -c $(SOURCES)	$(FLAGS)	

clean :
	rm -f $(EXEBIN) $(OBJECTS)

check:
	valgrind --leak-check=full $(EXEBIN)
