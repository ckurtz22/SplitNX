#include "splitter.hpp"

#include <fstream>
#include <iostream>

extern "C" {
    #include "mp3.h"
}

Splitter::Splitter(std::string filename)
{
    mp3MutInit();
    connected = false;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (R_FAILED(hidInitializeVibrationDevices(VibrationHandles, 2, CONTROLLER_HANDHELD, static_cast<HidControllerType>(TYPE_HANDHELD | TYPE_JOYCON_PAIR))))
        std::cout << "Vibration initialization failed" << std::endl;

    Reload(filename);

    v_start[0] = {0.3, 160.0, 0.3, 320.0};
    v_start[1] = {0.3, 160.0, 0.3, 320.0};
    v_stop[0] = {0.0, 160.0, 0.0, 320.0};
    v_stop[1] = {0.0, 160.0, 0.0, 320.0};
}

void Splitter::Reload(std::string filename) {
    std::fstream file;
    file.open(filename, std::fstream::in);
    file >> ip;
    file >> port;

    split s;
    splits.clear();
    while (file >> std::hex >> s.address >> std::dec >> s.op >> s.size >> s.value)
    {
        splits.push_back(s);
    }
    file.close();
    std::cout << splits.size() << std::endl;
}

void Splitter::Update()
{
    if (R_FAILED(hidSendVibrationValues(VibrationHandles, v_stop, 2)))
        std::cout << "stop vibration failed!" << std::endl;

    if (!connected)
        return;
    
    if (send_msg("getsplitindex\r\n", false) == -1) {
        connected = false;
        playMp3("/switch/SplitNX/disconnect.mp3");
    }
    else
    {
        std::string ind;
        if (recv_msg(ind) > 0)
        {
            size_t i = std::stoi(ind);
            if (i < 0 || i > splits.size())
                return;

            split s = splits[i];

            if (doOperator(readMemory(s.address, s.size), s.value, s.op))
                Split();
        }
    }
}

void Splitter::test_it() {
    std::fstream file;
    file.open("/splitnx.log", std::fstream::app);
    file << "Memory: " << readMemory(splits[0].address, splits[0].size) << std::endl;
    file.close();
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
    if (connected)
        playMp3("/switch/SplitNX/connect.mp3");
    else
        playMp3("/switch/SplitNX/disconnect.mp3");

}

void Splitter::Split()
{
    send_msg("startorsplit\r\n", connected);
}

void Splitter::Reset()
{
    send_msg("reset\r\n", connected);
}

void Splitter::Undo()
{
    send_msg("unsplit\r\n", connected);
}

void Splitter::Skip()
{
    send_msg("skipsplit\r\n", connected);
}

ssize_t Splitter::send_msg(std::string msg, bool vibrate)
{
    if (vibrate && R_FAILED(hidSendVibrationValues(VibrationHandles, v_start, 2)))
        std::cout << "start vibration failed!" << std::endl;
    return send(sock, msg.c_str(), msg.length(), 0);
}

ssize_t Splitter::recv_msg(std::string &msg)
{
    char buff[64];
    ssize_t ret = recv(sock, buff, 32, 0);
    msg = std::string(buff);
    return ret;
}

bool doOperator(u64 param1, u64 param2, std::string op)
{
    if (op == "==")
        return param1 == param2;
    else if (op == "!=")
        return param1 != param2;
    else if (op == "<")
        return param1 < param2;
    else if (op == ">")
        return param1 > param2;
    else if (op == "<=")
        return param1 <= param2;
    else if (op == ">=")
        return param1 >= param2;
    else
        return false;
}

u64 findHeapBase(Handle debugHandle)
{
    MemoryInfo memInfo = {0};
    u32 pageInfo;
    u64 lastAddr;
    do
    {
        lastAddr = memInfo.addr;
        svcQueryDebugProcessMemory(&memInfo, &pageInfo, debugHandle, memInfo.addr + memInfo.size);
    } while (memInfo.type != MemType_Heap && lastAddr < memInfo.addr + memInfo.size);
    if (memInfo.type != MemType_Heap)
        return 0;
    return memInfo.addr;
}

u64 readMemory(u64 address, size_t size)
{
    u64 val, pid;
    Handle handle;

    if (R_FAILED(pmdmntGetApplicationProcessId(&pid))) // TODO: Can't just return 0
        return 0;
    svcDebugActiveProcess(&handle, pid);
    svcReadDebugProcessMemory(&val, handle, findHeapBase(handle) + address, size / 8);
    svcCloseHandle(handle);

    return val;
}
