#pragma once

#include <string>

class GlobalRoute {
public:
    static GlobalRoute& getInstance();

    void processCommand(const std::string& command);


private:
    GlobalRoute() = default;
    ~GlobalRoute() = default;    
};