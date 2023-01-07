#include "splitter.hpp"
#include "dmntcht.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <fstream>
#include <errno.h>

Splitter::Splitter()
{

}

void Splitter::Reload(Splits s) {
    splits = s;
}

void Splitter::Update()
{
    if (!enabled || !this->IsConnected()) return;

    if (splits.loading.valid) 
    {
        SetLoading(do_operator(splits.loading));
    }

    size_t i = GetSplitIndex();
    if (i >= 0 && splits.splits.size() > i)
    {
        if (do_operator(splits.splits.at(i)))
        {
            Split();
        }
    } 
}

void Splitter::Connect(std::string ip, int port)
{
    close(sock);
    sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, opt | O_NONBLOCK);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_aton(ip.c_str(), &serv_addr.sin_addr);

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    fcntl(sock, F_SETFL, opt);
}

bool Splitter::IsConnected()
{
    if (!connected)
    {
    struct sockaddr_in addr; 
    socklen_t len; 
        connected = getpeername(sock, (struct sockaddr*)&addr, &len) == 0;
    }
    return connected;
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
    if (GetSplitTime() == "0:00") return ""; // If timer is not started then this could hang

    std::string name = "";
    if (recv_msg("getcurrentsplitname\r\n", name) > 0)
    {
        return name;
    }
    return "";
}

std::string Splitter::GetSplitTime()
{
    std::string time = "0:00.00";
    if (recv_msg("getcurrenttime\r\n", time) > 0)
    {
        time.erase(time.find_last_of('.'));
        return time;
    }
    return "0:00";
}

ssize_t Splitter::send_cmd(std::string cmd)
{
    if (!this->IsConnected()) return -1;

    m.lock();
    ssize_t ret = send(sock, cmd.c_str(), cmd.length(), 0);
    m.unlock();

    if (ret < 0) connected = false;
    return ret;
}

ssize_t Splitter::recv_msg(std::string cmd, std::string &resp)
{
    if (!this->IsConnected()) return -1;
    m.lock();
    ssize_t ret = send(sock, cmd.c_str(), cmd.length(), 0);

    if (this->IsConnected())
    {
        char buff[64];
            ret = recv(sock, buff, 32, 0);
        if (ret >= 0)
        {
            resp = std::string(buff);
            resp.erase(resp.find_first_of('\r'));
        }
    }
    m.unlock();

    if (ret < 0) connected = false;
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
    if (s.memory == "heap")
        offset = metadata.heap_extents.base;
    if (s.memory == "main") 
        offset = metadata.main_nso_extents.base;

    u64 val;
    dmntchtReadCheatProcessMemory(offset + s.address, &val, s.size / 8);
    return val & ((1 << s.size) - 1);
}