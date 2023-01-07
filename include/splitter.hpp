#pragma once

#include <vector>
#include <string>
#include <mutex>

#include <switch.h>
#include <json.hpp>
using json = nlohmann::json;


struct split
{
    std::string name;
    u64 address;
    u64 value;
    size_t size;
    std::string op;
    std::string memory;
    bool valid = false;

    split(json splitJson)
    {
        name = splitJson["name"].get<std::string>();
        address = std::stoi(splitJson["address"].get<std::string>(), 0, 16);
        value = splitJson["value"].get<int>();
        size = splitJson["size"].get<int>();
        op = splitJson["operator"].get<std::string>();
        memory = splitJson["memory"].get<std::string>();
        valid = true;
    }
    split()
    {
        valid = false;
    }
};

struct Splits
{
    std::string file;
    std::string game;
    std::string category;
    std::vector<split> splits;
    split loading;

    Splits(json splitsJson)
    {
        file = splitsJson["file"].get<std::string>();
        game = splitsJson["game"].get<std::string>();
        category = splitsJson["category"].get<std::string>();
        loading = split(splitsJson["loading"]);
        for (auto j : splitsJson["splits"])
        {
            splits.push_back(split(j));
        }
    }
    Splits() {}
};

class Splitter
{
public:
    Splitter();
    void Reload(Splits splits);
    void Update();
    void Connect(std::string ip, int port);
    void Split();
    void Reset();
    void Undo();
    void Skip();
    void SetLoading(bool);
    void EnableSplitting(bool en) { enabled = en; };
    Splits GetSplits() { return splits; };
    size_t GetSplitIndex();
    std::string GetSplitName();
    std::string GetSplitTime();
    bool TimerRunning();

    // int GetNumSplits() const { return splits.splits.size(); };
    bool IsConnected();


private:
    int sock;
    Splits splits;
    std::mutex m;
    bool enabled = false;
    bool connected = false;

    ssize_t send_cmd(std::string cmd);
    ssize_t recv_msg(std::string cmd, std::string& resp);
    bool do_operator(split s);
    u64 read_memory(split s);
};
