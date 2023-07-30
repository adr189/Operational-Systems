CC = gcc
OBJ_dir = obj
BIN_dir = bin
INC_dir = include
OBJECTOS = client.o driver.o memory.o main.o restaurant.o process.o

client.o = memory.h client.h main.h
driver.o = driver.h memory.h main.h
memory.o = memory.h
process.o = client.h driver.h memory.h main.h restaurant.h
restaurant.o = memory.h main.h
main.o = memory.h process.h

CFLAGS = -Wall -I $(INC_dir)
LIBS = -lrt

vpath %.o $(OBJ_dir)

magnaeats: $(OBJECTOS)
	$(CC) $(addprefix $(OBJ_dir)/,$(OBJECTOS)) -o $(BIN_dir)/magnaeats $(LIBS)

%.o: src/%.c $($@)
	$(CC) $(CFLAGS) -o $(OBJ_dir)/$@ -c $<

clean:
	rm –f *.o magnaeats
	rm –f magnaeats
