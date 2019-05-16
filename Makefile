ninjas: ninjas.c
	gcc -pthread -o ninjas ninjas.c -lm

intersection: intersection.c
	gcc -pthread -o intersection intersection.c

all: ninjas intersection

clean:
	rm ninjas intersection
