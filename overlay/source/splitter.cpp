#include "splitter.hpp"
#include "dmntcht.h"

#include <fstream>
#include <iostream>
#include <sys/time.h>


extern std::fstream logger;

Splitter::Splitter(std::string filename)
{
    connected = false;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    Reload(filename);
}

void Splitter::Reload(std::string filename) {
    std::fstream file;
    file.open(filename, std::fstream::in);
    file >> ip;
    file >> port;

    split s;
    s.valid = true;
    splits.clear();
    while (file >> s.type >> std::hex >> s.address >> std::dec >> s.op >> s.size >> s.value)
    {
        if (s.op.find("load") != std::string::npos) {
            s.op = s.op.substr(4);
            loading = s;
        }
        else 
            splits.push_back(s);
    }
    file.close();
    std::cout << splits.size() << std::endl;
}

void Splitter::Update()
{
    if (!connected || send_msg("getsplitindex\r\n") == -1) 
    {
        connected = false;
        return;
    }

    if (loading.valid) 
    {
        SetLoading(doOperator(loading));
    }

    std::string ind;
    if (recv_msg(ind) > 0)
    {        
        size_t i = std::stoi(ind);
        if (i >= 0 && splits.size() > i && doOperator(splits.at(i)))
            Split();
    }
}

// void Splitter::test_it() {
//     std::fstream file;
//     file.open("/splitnx.log", std::fstream::app);
//     file << "Memory: " << readMemory(splits[0].address, splits[0].size, splits[0].type) << std::endl;
//     file.close();
// }

bool Splitter::Connect()
{
    close(sock);
    sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_aton(ip.c_str(), &serv_addr.sin_addr);

    // struct timeval tv = {
    //     .tv_sec = 0,
    //     .tv_usec = 100000
    // };

    // setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    // setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    connected = (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0);

    return connected;
}

void Splitter::Split()
{
    send_msg("startorsplit\r\n");
}

void Splitter::Reset()
{
    send_msg("reset\r\n");
}

void Splitter::Undo()
{
    send_msg("unsplit\r\n");
}

void Splitter::Skip()
{
    send_msg("skipsplit\r\n");
}

void Splitter::SetLoading(bool loadingState) 
{
    if (loadingState) 
        send_msg("pausegametime\r\n");
    else
        send_msg("unpausegametime\r\n");
    
}

std::string Splitter::GetSplitName()
{
    auto time = GetSplitTime();
    if (time == "0:00") return ""; // If timer is not started then this will hang
    if (send_msg("getcurrentsplitname\r\n") > 0)
    {
        std::string name;
        if (recv_msg(name) > 0)
        {
            return name;
        }
    }
    return "";
}

std::string Splitter::GetSplitTime()
{
    if (send_msg("getcurrenttime\r\n") > 0)
    {
        std::string time;
        if (recv_msg(time) > 0)
        {
            time.erase(time.find_last_of('.'));
            return time;
        }
    }
    return "";
}

ssize_t Splitter::send_msg(std::string msg)
{
    return send(sock, msg.c_str(), msg.length(), 0);
}

ssize_t Splitter::recv_msg(std::string &msg)
{
    char buff[64];
    ssize_t ret = recv(sock, buff, 32, 0);
    msg = std::string(buff);
    msg.erase(msg.find_first_of('\r'));
    return ret;
}

bool doOperator(split s)
{
    u64 val = readMemory(s.address, s.size, s.type);

    if (s.op == "==")
        return val == s.value;
    else if (s.op == "!=")
        return val != s.value;
    else if (s.op == "<")
        return val < s.value;
    else if (s.op == ">")
        return val > s.value;
    else if (s.op == "<=")
        return val <= s.value;
    else if (s.op == ">=")
        return val >= s.value;
    else
        return false;
}

u64 readMemory(u64 address, size_t size, std::string type)
{
    dmntchtForceOpenCheatProcess();
    DmntCheatProcessMetadata metadata;
    dmntchtGetCheatProcessMetadata(&metadata);
    u64 offset = 0;
    if (type == "heap")
        offset = metadata.heap_extents.base;
    if (type == "main") 
        offset = metadata.main_nso_extents.base;

    u64 val;
    dmntchtReadCheatProcessMemory(offset + address, &val, size / 8);
    return val & ((1 << size) - 1);
}