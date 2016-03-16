CC = arm-linux-gnueabihf-gcc
CFLAGS = -O2 -Wall
INC = -I../../../INCLUDE/NATIVE
EXEC = dpram_ctrl

$(EXEC): $(EXEC).c
	$(CC) $(CFLAGS) $(INC) $^ -o $@

clean:
	rm $(EXEC)
