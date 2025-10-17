TARGET = main

SRCS = ubus.c get_data.c main.c 

OBJS = $(SRCS:.c=.o)

CC = gcc
CFLAGS = -Wall -Wextra -g
DFLAGS = -lubox -lubus

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^  $(DFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< 

run:
	sudo ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
