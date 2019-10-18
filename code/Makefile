CC = gcc 
SRC= TinyFile.c ../snappy-c/snappy.c ../snappy-c/map.c ../snappy-c/util.c
OBJ= $(SRC:.c=.o)
LIBS = -lrt -pthread -lyaml



all: client server tester

client: $(OBJ) client.o
	$(CC) $(SRC) client.c -o client $(LIBS)

server: $(OBJ) server.o
	$(CC) $(SRC) server.c -o server $(LIBS)	

tester: tester.o
	$(CC) tester.c -o tester


debug_client: $(OBJ) client.o
	$(CC) -g $(SRC) client.c -o client $(LIBS)

debug_server:$(OBJ) server.o
	$(CC) -g $(SRC) server.c -o server $(LIBS)	

debug: debug_client debug_server

clean:
	@rm *.o client server ../output/stress_testing_sync_1_segment.csv \
		../output/stress_testing_sync_3_segment.csv ../output/stress_testing_sync_5_segment.csv \
		../output/stress_testing_sync_1_segment.png ../output/stress_testing_sync_3_segment.png \
		../output/stress_testing_sync_5_segment.png
	@echo Cleaned!

