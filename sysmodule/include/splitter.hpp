#pragma once

#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <switch.h>

bool doOperator(u64 param1, u64 param2, std::string op);
u64 readMemory(u64 address, size_t size, std::string type);

struct split {
    u64 address;
    u64 value;
    size_t size;
    std::string op;
    std::string type;
    bool valid = false;
};

class Splitter {
public:
    Splitter(std::string filename);
    void Reload(std::string filename);
    void Update();
    void Connect();
    void Split();
    void Reset();
    void Undo();
    void Skip();
    void SetLoading(bool);

    void debug_first_mem();

private:
    bool connected;
    int sock;
    int port;
    std::string ip;
    std::vector<split> splits;
    split loading;

    ssize_t send_msg(std::string);
    ssize_t recv_msg(std::string&);
};
