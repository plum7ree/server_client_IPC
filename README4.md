### server_client_IPC

##protocol

server creates: one global message queue, one sharedmem, one global semaphore

client creates : two private message queue, two private semaphore -> send pid to server and server will open also.

1. client use global queue when connect to server : telling only pid of client, size: 4byte  ex) sprintf(msgbuff, "%04d", pid);
	a. Since client is just sending "pid" through global queue, server will create semaphore 
		and two private message queues same as client: ex) sprintf(msgbuff, "/%04d%03d", pid, index);

2. two semaphore for per client and server conection: sem_modif, sem_allow_transf
	a. their path is 8bytes string : ex) sprintf(msgbuff, "/%04d%03d", pid, index);
	b. server will also sem_open() same path once it recieve "pid" of client



##compile

make server; //for server
make client; //for client

// or just
make all
