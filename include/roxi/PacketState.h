#pragma once

namespace roxi {

class PacketState{
public:
    PacketState() : state_(0), current_(0), current_cmd_size_(0),
                    key_cmd_idx_(0), current_cmd_idx_(0), max_cmd_count_(0) {}

public:
    int state_;
    int current_;
    int current_cmd_idx_;
    int current_cmd_size_;
    int max_cmd_count_;
    int key_cmd_idx_;

    std::string command_;
    std::string key_;
};

}
