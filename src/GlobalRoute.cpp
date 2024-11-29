#include "GlobalRoute.h"
#include <iostream>
#include "MemMonitor/MemCommandHandler.h"
#include "FileInfo.h"

GlobalRoute& GlobalRoute::getInstance() {
    static GlobalRoute instance;
    return instance;
}

void GlobalRoute::processCommand(const std::string& command) {
    if (command == "mem") {
        MemCommandHandler::handleMemoryCommands();
    }
    else if (command == "file") {
        FileInfo fileInfo;
        // File information processing
    }
    else if (command == "disk") {
        std::cout << "Disk information feature not yet implemented\n";
    }
    else if (command == "network") {
        std::cout << "Network information feature not yet implemented\n";
    }
    else {
        std::cout << "Unknown command, available commands:\n"
                 << "mem - Display memory information\n"
                 << "file - Display file information\n"
                 << "disk - Display disk information\n"
                 << "network - Display network information\n";
    }
}
