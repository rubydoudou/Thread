# Makefile to compile UMIX Programming Assignment 4 (pa4) [updated: 12/20/19]

LIBDIR = $(UMIXPUBDIR)/lib

CC 	= cc 
FLAGS 	= -g -L$(LIBDIR) -lumix4

PA4 =	pa4a pa4b pa4c tests
pa4:	$(PA4)

pa4a:	pa4a.c aux.h umix.h
	$(CC) $(FLAGS) -o pa4a pa4a.c

pa4b:	pa4b.c aux.h umix.h mycode4.h mycode4.o
	$(CC) $(FLAGS) -o pa4b pa4b.c mycode4.o

pa4c:	pa4c.c aux.h umix.h mycode4.h mycode4.o
	$(CC) $(FLAGS) -o pa4c pa4c.c mycode4.o

mycode4.o:	mycode4.c aux.h umix.h mycode4.h
	$(CC) $(FLAGS) -c mycode4.c

tests:	pa4tests.c aux.h umix.h mycode4.h mycode4.o
	$(CC) $(FLAGS) $(OPTION) -o tests pa4tests.c mycode4.o
clean:
	rm -f *.o $(PA4)
