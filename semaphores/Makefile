#changed make file for semaphores
OBJ = semaphores.o queue.o
all: run
run: $(OBJ)
	gcc $(OBJ) -pthread -o run
$(OBJ): myQueue.h
.PHONY: clean
clean:
	rm -f *.o run