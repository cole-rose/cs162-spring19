all: main map wc

main: main.o
	gcc -g main.o -o main

map: map.o
	gcc -g map.o -o map

wc: wc.o
	gcc -g wc.o -o wc

main.o: main.c
	gcc -g -c main.c

map.o: map.c
	gcc -g -c map.c
	
wc.o: wc.c
	gcc -g -c wc.c

clean:
	rm -rf main.o wc.o map.o
