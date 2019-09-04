EXEC = bin/$(LBITS)/engine

CC = g++
CFLAGS = -O1 -Wno-attributes

FILES = \
	src/main.cpp \
	src/render.cpp \


ifeq ($(OS), Windows_NT)

LBITS = $(shell getconf LONG_BIT)

# Windows

LIBS = -lmingw32 -lSDL2 -lOpenGL32
EXEC := $(EXEC).exe
FILES += include/GL/glew.c

CFLAGS += -DWINDOWS -Iinclude
CFLAGS += -Llib/$(LBITS)

else

# Linux

LIBS = -lSDL2 -lGL -lGLEW
EXEC := $(EXEC).out
CFLAGS += -DLINUX

endif

all: $(EXEC)
	@echo Up to date

$(EXEC): $(FILES)
	@echo Rebuilding
	$(CC) $^ $(CFLAGS) $(LIBS) -o $@

run: all
	$(EXEC)

