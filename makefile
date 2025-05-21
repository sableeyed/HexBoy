CC ?= gcc
RC = windres
CFLAGS = -Wall -g -mwindows -I$(INC)

SRC = src
INC = include
BUILD = build
BIN = bin
RES = res

SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(SRCS))
RESRC = $(SRC)/resource.rc
RESOBJ = $(BUILD)/resource.o

TARGET := $(BIN)/hexboy.exe

all: $(TARGET)

$(TARGET): $(OBJS) $(RESOBJ)
	if not exist $(BIN) mkdir $(BIN)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRC)/%.c
	if not exist $(BUILD) mkdir $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(RESOBJ): $(RESRC)
	if not exist $(BUILD) mkdir $(BUILD)
	$(RC) -I$(INC) -i $< -o $@

clean:
	del /F /Q $(BUILD)\* & del /F /Q $(BIN)\hexboy.exe