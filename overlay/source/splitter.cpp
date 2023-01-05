#include "splitter.hpp"
#include "dmntcht.h"

#include <fstream>

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
}

void Splitter::Update()
{
    if (!connected) return;

    if (loading.valid) 
    {
        SetLoading(do_operator(loading));
    }

    size_t i = GetSplitIndex();
    if (i >= 0 && splits.size() > i)
    {
        if (do_operator(splits.at(i)))
        {
            Split();
        }
    } 
}

void Splitter::Connect()
{
    close(sock);
    sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_aton(ip.c_str(), &serv_addr.sin_addr);

    connected = (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0);

    if (connected && loading.valid)
    {
        send_cmd("initgametime\r\n");
    }
}

void Splitter::Split()
{
    send_cmd("startorsplit\r\n");
}

void Splitter::Reset()
{
    send_cmd("reset\r\n");
}

void Splitter::Undo()
{
    send_cmd("unsplit\r\n");
}

void Splitter::Skip()
{
    send_cmd("skipsplit\r\n");
}

void Splitter::SetLoading(bool loadingState) 
{
    if (loadingState) 
        send_cmd("pausegametime\r\n");
    else
        send_cmd("unpausegametime\r\n");
    
}

size_t Splitter::GetSplitIndex()
{
    std::string ind = "-1"; // safe default value if not connected
    recv_msg("getsplitindex\r\n", ind);
    return std::stoi(ind);
}

std::string Splitter::GetSplitName()
{
    if (GetSplitTime() == "0:00") return ""; // If timer is not started then this will hang

    std::string name = "";
    recv_msg("getcurrentsplitname\r\n", name);
    return name;
}

std::string Splitter::GetSplitTime()
{
    std::string time;
    recv_msg("getcurrenttime\r\n", time);
    time.erase(time.find_last_of('.'));
    return time;
}

ssize_t Splitter::send_cmd(std::string cmd)
{
    if (!connected) return -1;

    m.lock();
    ssize_t ret = send(sock, cmd.c_str(), cmd.length(), 0);
    if (ret < 0) connected = false;
    m.unlock();

    return ret;
}

ssize_t Splitter::recv_msg(std::string cmd, std::string &resp)
{
    if (!connected) return -1;
    
    m.lock();
    ssize_t ret = send(sock, cmd.c_str(), cmd.length(), 0);
    if (ret < 0) connected = false;

    if (connected)
    {
        char buff[64];
        ret = recv(sock, buff, 32, 0);

        if (ret < 0) connected = false;
        else
        {
            resp = std::string(buff);
            resp.erase(resp.find_first_of('\r'));
        }
    }
    m.unlock();

    return ret;
}

bool Splitter::do_operator(split s)
{
    u64 val = read_memory(s);

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

u64 Splitter::read_memory(split s)
{
    dmntchtForceOpenCheatProcess();
    DmntCheatProcessMetadata metadata;
    dmntchtGetCheatProcessMetadata(&metadata);

    u64 offset = 0;
    if (s.type == "heap")
        offset = metadata.heap_extents.base;
    if (s.type == "main") 
        offset = metadata.main_nso_extents.base;

    u64 val;
    dmntchtReadCheatProcessMemory(offset + s.address, &val, s.size / 8);
    return val & ((1 << s.size) - 1);
}