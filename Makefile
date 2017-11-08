all: link

link: compile
	gcc *.o -o proj3 -lpthread

compile:
	gcc -c main.c

clean:
	rm *.o
	rm proj3
