CC=$(CROSS_COMPILE)gcc
CFLAGS= -g -Wall
SOURCE=$(wildcard *.c)
OBJS=$(SOURCE:.c=.o)


.PHONY: all clean

writer: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	@rm -rf writer $(OBJS)
