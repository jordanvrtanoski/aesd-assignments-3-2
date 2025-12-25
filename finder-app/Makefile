CROSS_COMPILE ?=

CC      := $(CROSS_COMPILE)gcc
CFLAGS  := -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L
TARGET  := writer
SRCS    := $(shell find . -name "*.c")
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

