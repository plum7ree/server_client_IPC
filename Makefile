CC = gcc
SRC= TinyFile.c snappy-c/snappy.c snappy-c/map.c snappy-c/util.c
OBJ= $(SRC:.c=.o)
LIBS = -lrt -pthread -I./libyaml/include/

client: $(OBJ) client.o
	$(CC) $(SRC) libyaml.so client.c -o client $(LIBS)

server: $(OBJ) server.o
	$(CC) $(SRC) server.c -o server $(LIBS)	

clean:
	rm client server || true

all: client server

debug_client: $(OBJ) client.o
	$(CC) -g $(SRC) client.c -o client $(LIBS)

debug_server:$(OBJ) server.o
	$(CC) -g $(SRC) server.c -o server $(LIBS)	

debug: debug_client debug_server

