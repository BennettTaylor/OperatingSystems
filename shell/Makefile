override CFLGAGS := -Wall -Werror -std=gnu99 -O0 -g $(CFLAGS) -I.

myshell : myshell.o myshell_parser.o
		gcc -o myshell myshell.o myshell_parser.o

myshell.o : myshell.c myshell_parser.h

myshell_parser.o : myshell_parser.c myshell_parser.h




clean:
		rm -f *.o myshell myshell.o myshell_parser.o
