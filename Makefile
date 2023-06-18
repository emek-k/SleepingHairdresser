OBJ = main.o queue.o
all: run
run: $(OBJ)
	gcc $(OBJ) -o run
$(OBJ): myQueue.h
.PHONY: clean
clean:
	rm -f *.o run