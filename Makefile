OBJ = main.c queue.c
output: main.c
	gcc $(OBJ) -o run

main.o: main.c
	gcc -c main.c

queue.o: queue.c myQueue.h
	gcc -c queue.c
	
.PHONY: clean
clean:
	rm -f *.o run
