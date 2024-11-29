#include "GlobalRoute.h"

#include <iostream>

#include "MemMonitor.h"
#include "FileInfo.h"

GlobalRoute& GlobalRoute::getInstance() {
    static GlobalRoute instance;
    return instance;
}

void GlobalRoute::processCommand(const std::string& command) {
    if (command == "mem") {
        MemMonitor memMonitor;
        std::cout << memMonitor.getMemOverview() << std::endl;
    }
    else if (command == "file") {
        FileInfo fileInfo;
        // File information processing
    }
    else if (command == "disk") {
        // Disk information processing
        std::cout << "Disk information feature not yet implemented" << std::endl; 
    }
    else if (command == "network") {
        // Network information processing
        std::cout << "Network information feature not yet implemented" << std::endl;
    }
    else {
        std::cout << "Unknown command, available commands:\n";
        std::cout << "mem - Display memory information\n";
        std::cout << "file - Display file information\n"; 
        std::cout << "disk - Display disk information\n";
        std::cout << "network - Display network information" << std::endl;
    }
    
}
