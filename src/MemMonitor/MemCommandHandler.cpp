#include "MemCommandHandler.h"
#include <iostream>

void MemCommandHandler::showMemoryMenu() {
    std::cout << "\nMemory Information Options:\n"
              << "1. Overview (mem)\n"
              << "2. Process Memory (process)\n"
              << "3. Memory Performance (perf)\n"
              << "4. Page File Information (page)\n"
              << "0. Back to main menu\n"
              << "Enter option: ";
}

void MemCommandHandler::handleMemoryCommands() {
    MemMonitor monitor;
    std::string command;

    while (true) {
        showMemoryMenu();
        std::getline(std::cin, command);

        std::cout << "Received command: " << command << std::endl;

        if (command == "0") {
            break;
        }

        processMemoryCommand(command, monitor);
    }
}

void MemCommandHandler::processMemoryCommand(const std::string& command, MemMonitor& monitor) {
    std::cout << "Processing memory command: " << command << std::endl;
    if (command == "1") {
        std::cout << monitor.getMemOverview();
    }
    else if (command == "2") {
        std::cout << monitor.getDetailedMemInfo("process");
    }
    else if (command == "3") {
        std::cout << monitor.getDetailedMemInfo("perf");
    }
    else if (command == "4") {
        std::cout << monitor.getDetailedMemInfo("page");
    }
    else {
        std::cout << "Invalid memory command\n";
    }
} 