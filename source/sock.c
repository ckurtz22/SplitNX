#include "sock.h"


bool tryConnect(int* sock, const char* address, int port) {
    close(*sock);
    *sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_aton(address, &serv_addr.sin_addr); 
	return connect(*sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0;
}

void send_msg(int sock, const char* msg) {
    printf("Sending message: \n%s\n", msg);
	send(sock, msg, strlen(msg), 0);
}
