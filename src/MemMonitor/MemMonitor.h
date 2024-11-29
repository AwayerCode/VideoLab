#pragma once

#include <string>
#include <vector>

class MemMonitor {
public:
    MemMonitor();
    ~MemMonitor();

public:
    std::string getMemOverview();
    std::string getDetailedMemInfo(const std::string& command);
    static void showMemoryMenu();

private:
    std::string getProcessMemoryInfo();
    std::string getMemoryPerformance();
    std::string getPageFileInfo();
};

