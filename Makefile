OBJ = main.o myThredLib.o
all: run
run: $(OBJ)
	gcc $(OBJ) -o run
$(OBJ): myThredHeader.h
.PHONY: clean
clean:
	rm -f *.o run