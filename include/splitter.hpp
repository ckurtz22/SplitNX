#pragma once

#include <vector>
#include <string>
#include <mutex>

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

class Splitter
{
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
    size_t GetSplitIndex();
    std::string GetSplitName();
    std::string GetSplitTime();

    std::string GetIp() const { return ip; };
    int GetPort() const { return port; };
    int GetNumSplits() const { return splits.size(); };
    bool IsConnected() const { return connected; };


private:
    bool connected;
    int sock;
    int port;
    std::string ip;
    std::vector<split> splits;
    split loading;

    std::mutex m;

    ssize_t send_cmd(std::string cmd);
    ssize_t recv_msg(std::string cmd, std::string& resp);
    bool do_operator(split s);
    u64 read_memory(split s);
};
