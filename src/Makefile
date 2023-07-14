# Rui Daniel, Afonso Alemao 5-04-2022
#
# Compile program: make
# To delete .o and exe: make clean

#   Compiler flags

CC = gcc
CFLAGS = -Wall #-ggdb3
#   Sources
SOURCES = main.c tcp.c udp.c aux.c commands.c 

#   Objects  ('make' automatically compiles .c to .o)
OBJECTS = main.o tcp.o udp.o aux.o commands.o 

ring: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

main.o: main.c 

aux.o: aux.c aux.h

tcp.o: tcp.c tcp.h aux.h

udp.o: udp.c udp.h aux.h commands.h

commands.o: commands.c commands.h aux.h tcp.h udp.h

clean:
	rm *.o ring
