CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lncurses -lm
TARGET  = network_monitor
SRCS    = main.c collector.c bandwidth.c display.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo ""
	@echo "  Build successful! Run with:  sudo ./$(TARGET)"
	@echo ""

%.o: %.c network_monitor.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install:
	cp $(TARGET) /usr/local/bin/
