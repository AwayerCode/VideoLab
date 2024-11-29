#pragma once
#include "MemMonitor.h"
#include <string>

class MemCommandHandler {
public:
    static void handleMemoryCommands();
    static void showMemoryMenu();

private:
    static void processMemoryCommand(const std::string& command, MemMonitor& monitor);
}; 