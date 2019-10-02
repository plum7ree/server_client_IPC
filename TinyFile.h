#define SHMPATH "/shm_obj1" // naming convention: /somename. Not ./somename
#define SEMPATH "/sem_obj1"
#define SEMPATH2 "/sem_obj2"
#define SEMPATH3 "/sem_obj3"
#define MQPATH "/mq_obj1"

#define SEGSIZE 10
#define MSGSIZE 4

/**packet: 
Header : | method(1byte) | PID (4byte) | FILENAME_int (4byte) | SIZE(4byte) ) | => 13byte

**/
/** Methods:
FILE (char value: 1) | PID (4byte) | FILENAME_int (4byte) | TOTALSIZE(4byte) | no content
FILE2(char value: 2) | PID (4byte) | FILENAME_int (4byte) | size of fraction(4byte) | content

**/

#define MHD_FILE  '1'
#define MHD_FILE2 '2'

#define HEADER_SIZE 13


char *filepath = "peda-session-client.txt";




