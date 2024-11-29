#pragma once

#include <string>
#include <vector>

class MemMonitor {
public:
    MemMonitor();
    ~MemMonitor();

public:
    std::string getMemOverview();
};

