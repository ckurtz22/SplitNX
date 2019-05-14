#pragma once

#include <vector>
#include <string>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

extern "C"
{
#include "mem.h"
}


bool doOperator(u64 param1, u64 param2, std::string op);

struct split
{
    u64 address;
    u64 value;
    size_t size;
    std::string op;
};

class Splitter
{
public:
    Splitter(std::string filename);
    void Update();
    void Connect();
    void Split();
    void Reset();
    void Undo();
    void Skip();

private:
    int sock;
    int port;
    std::string ip;
    std::vector<split> splits;

    ssize_t send_msg(std::string);
    ssize_t recv_msg(std::string&);
};
