


void connectClient() {
	//1. send message with sem_allow_msg, sem_msg_sent

}

void sendFile() {
	while(filesize) {
		// 1. read a chunk from file
		readChunk
		sem_wait(sem_allow_msg)
		sendMsg
		sem_post(sem_msg_sent)

		// 2. send a chunk to shared mem
		sem_wait(sem_allow_transf)
		sendToSM
		sem_post(sem_modif)

	}
}

void receiveFile() {
	while(filesize) {
		sem_wait(sem_modif_by_server)
		receiveFromSM

		sem_post(sem_modif_by_server)
	}
}


int main() {

	connectClient();

	



	readMsg()
	filesize 
	filenumber
	
		

	


}