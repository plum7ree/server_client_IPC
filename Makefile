CC = gcc
SRC= server.c snappy-c/snappy.c snappy-c/map.c snappy-c/util.c
OBJ= $(SRC:.c=.o)
LIBS = -lrt -pthread

all: $(OBJ)
	$(CC) $(SRC) -o server $(LIBS)


