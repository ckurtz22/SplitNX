#pragma once

#include <vector>
#include <string>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <switch.h>


struct split
{
    u64 address;
    u64 value;
    size_t size;
    std::string op;
    std::string type;
    bool valid = false;
};

bool doOperator(split s);
u64 readMemory(u64 address, size_t size, std::string type);


class Splitter
{
public:
    Splitter(std::string filename);
    void Reload(std::string filename);
    void Update();
    bool Connect();
    void Split();
    void Reset();
    void Undo();
    void Skip();
    void SetLoading(bool);
    std::string GetSplitName();
    std::string GetSplitTime();

    std::string GetIp() const { return ip; };
    int GetPort() const { return port; };
    int GetNumSplits() const { return splits.size(); };
    bool IsConnected() const { return connected; };

    void test_it();

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
