#include "sock.h"

int sock;

bool tryConnect(const char* address, int port) {
    close(sock);
    sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_aton(address, &serv_addr.sin_addr); 

	return connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0;
}

ssize_t send_msg(const char* msg) {
	return send(sock, msg, strlen(msg), 0);
}

ssize_t recv_msg(char* msg, size_t msg_size) {
	return recv(sock, msg, msg_size, 0);
}
