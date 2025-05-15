CC ?= gcc
CFLAGS = -Wall -g -mwindows

SRC = src
INC = include
BUILD = build
BIN = bin

SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(SRCS))

TARGET := $(BIN)/hexboy.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	if not exist $(BIN) mkdir $(BIN)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRC)/%.c
	if not exist $(BUILD) mkdir $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /F /Q $(BUILD)\* & del /F /Q $(BIN)\masochistboy.exe