#include "splitter.hpp"

#include <fstream>
#include <iostream>


Splitter::Splitter(std::string filename) {
    std::fstream file;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    file.open(filename);

    file >> ip;
    file >> port;

    split s;
    while (file >> std::hex >> s.address >> s.op >> s.size >> s.value) {
        splits.push_back(s);
    }
    file.close();
}

void Splitter::Update() {
    send_msg("getsplitindex\r\n");
    std::string ind;
    if (recv_msg(ind) > 0) {
        int i = std::stoi(ind);
        if (i < 0 || i > splits.size()) return;

        split s = splits[i];
        if (doOperator(readMemory(s.address, s.size), s.value, s.op));
    }
}

void Splitter::Connect() {
    close(sock);
    sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_aton(ip.c_str(), &serv_addr.sin_addr); 

	connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
}

void Splitter::Split() {
    send_msg("startorsplit\r\n");
}

void Splitter::Reset() {
    send_msg("reset\r\n");
}

void Splitter::Undo() {
    send_msg("unsplit\r\n");
}

void Splitter::Skip() {
    send_msg("skipsplit\r\n");
}

ssize_t Splitter::send_msg(std::string msg) {
    return send(sock, msg.c_str(), msg.length(), 0);
}

ssize_t Splitter::recv_msg(std::string &msg) {
    char buff[32];
    ssize_t ret = recv(sock, buff, 32, 0);
    msg = std::string(buff);
    return ret;
}

bool doOperator(u64 param1, u64 param2, std::string op) {
        if(op == "eq") return param1 == param2;
        else if(op == "ne") return param1 != param2;
        else if(op == "lt") return param1 < param2;
        else if(op == "gt") return param1 > param2;
        else if(op == "le") return param1 <= param2;
        else if(op == "ge") return param1 >= param2;
        else return false;
}