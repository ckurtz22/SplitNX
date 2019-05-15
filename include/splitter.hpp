#pragma once

#include <vector>
#include <string>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <switch.h>



bool doOperator(u64 param1, u64 param2, std::string op);
u64 findHeapBase(Handle debugHandle);
u64 readMemory(u64 address, size_t size);

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
    bool connected;
    int sock;
    int port;
    std::string ip;
    std::vector<split> splits;
    
    u32 VibrationHandles[2];
    HidVibrationValue v_start[2], v_stop[2];

    ssize_t send_msg(std::string, bool vibrate);
    ssize_t recv_msg(std::string&);
    void buzz_good();
    void buzz_bad();
};
