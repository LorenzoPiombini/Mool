TARGET = ./mem
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))


OBJlibmem = obj/memory.o  

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

LIBNAMEmem = mem
LIBDIR = /usr/local/lib
INCLUDEDIR = /usr/local/include
SHAREDLIBmem = lib$(LIBNAMEmem).so

default: $(TARGET)


clean: 
	rm obj/*.*
	rm *.so
	rm $(TARGET)
	sudo rm -f $(INCLUDEDIR)/memory.h
	sudo rm -f $(LIBDIR)/$(SHAREDLIBmem)

library:
	sudo gcc -Wall -fPIC -shared -o $(SHAREDLIBmem) $(OBJlibmem)



$(TARGET): $(OBJ)
	gcc -o $@ $? -llog -fpie -pie -z relro -z now -z noexecstack -fsanitize=address



obj/%.o : src/%.c
	gcc  -Wall -Wextra -Walloca -Warray-bounds -Wnull-dereference -g3 -c $< -o $@ -Iinclude -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIC -fsanitize=address


install: default library
	sudo install -d $(INCLUDEDIR)
	sudo install -m 644 include/memory.h $(INCLUDEDIR)/
	sudo install -m 755 $(SHAREDLIBmem) $(LIBDIR)
	sudo ldconfig
