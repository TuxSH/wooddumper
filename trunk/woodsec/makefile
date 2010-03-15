WFLAGS=-Wall -W -Wpointer-arith -Winline -Wno-strict-aliasing -Wno-unused -Werror
CPPFLAGS=-c -mtune=core2 -O3 $(WFLAGS) -fomit-frame-pointer -fstrict-aliasing -fno-exceptions -fno-rtti -fno-threadsafe-statics
LFLAGS=-s
LIBS=

RM=rm -f
MV=mv -f
CP=cp -f
CC=gcc
DLLTOOL=dlltool
M4=m4
MKDIR=mkdir -p

OBJS=encryption.o woodsec.o
TARGET=woodsec.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -o $@ $^ $(LIBS)

encryption.o: encryption.cpp header.h little.h types.h woodsec.h
	$(CC) $(CPPFLAGS) $< -o $@

woodsec.o: woodsec.cpp
	$(CC) $(CPPFLAGS) $< -o $@

clear:
	$(RM) $(OBJS)
	$(RM) $(TARGET)
