CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = mypreprocessor

SOURCES = main.c comment_removal.c file_inclusion.c macro_handler.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c preprocessor.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) temp_comments.c temp_includes.c

.PHONY: all clean
