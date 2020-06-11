
#Creates the httpserver
#Jake Armendariz
#Makefile-- creates an executable: httpserver
SOURCES	 = loadbalancer.c	lib.c	socket.c
OBJECTS	 = loadbalancer.o	lib.o	socket.o
HEADERS	 = loadbalancer.h
EXEBIN	 = loadbalancer
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
