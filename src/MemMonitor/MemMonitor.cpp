#include "MemMonitor.h"
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <winternl.h>

MemMonitor::MemMonitor() {
}

MemMonitor::~MemMonitor() {
}

std::string MemMonitor::getMemOverview() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    DWORDLONG totalVirtualMem = memInfo.ullTotalVirtual;

    std::stringstream ss;
    ss << "Memory Overview:\n"
       << "Total Physical Memory: " << totalPhysMem / (1024 * 1024) << " MB\n"
       << "Used Physical Memory: " << physMemUsed / (1024 * 1024) << " MB\n"
       << "Total Virtual Memory: " << totalVirtualMem / (1024 * 1024) << " MB\n";

    return ss.str();
}

std::string MemMonitor::getDetailedMemInfo(const std::string& command)
{
    std::stringstream ss;

    if (command == "all") {
        ss << getMemOverview()
           << "\n"
           << getProcessMemoryInfo() 
           << "\n"
           << getMemoryPerformance()
           << "\n" 
           << getPageFileInfo();
    }
    else if (command == "overview") {
        ss << getMemOverview();
    }
    else if (command == "process") {
        ss << getProcessMemoryInfo();
    }
    else if (command == "performance") {
        ss << getMemoryPerformance(); 
    }
    else if (command == "pagefile") {
        ss << getPageFileInfo();
    }
    else {
        ss << "Invalid command. Available commands:\n"
           << "all - Show all memory information\n"
           << "overview - Show memory overview\n"
           << "process - Show process memory details\n"
           << "performance - Show memory performance\n"
           << "pagefile - Show page file information\n";
    }

    return ss.str();
}


std::string MemMonitor::getProcessMemoryInfo() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    ZeroMemory(&pmc, sizeof(pmc));
    if (!GetProcessMemoryInfo(GetCurrentProcess(), 
                            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), 
                            sizeof(pmc))) {
        return "Failed to get process memory info\n";
    }
    
    std::stringstream ss;
    ss << "Process Memory Details:\n"
       << "Working Set Size: " << pmc.WorkingSetSize / (1024 * 1024) << " MB\n"
       << "Private Usage: " << pmc.PrivateUsage / (1024 * 1024) << " MB\n"
       << "Peak Working Set: " << pmc.PeakWorkingSetSize / (1024 * 1024) << " MB\n";
    return ss.str();
}

std::string MemMonitor::getMemoryPerformance() {
    PERFORMANCE_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    if (!GetPerformanceInfo(&pi, sizeof(pi))) {
        return "Failed to get performance info\n";
    }
    
    std::stringstream ss;
    ss << "Memory Performance:\n"
       << "Commit Total: " << pi.CommitTotal * pi.PageSize / (1024 * 1024) << " MB\n"
       << "Commit Limit: " << pi.CommitLimit * pi.PageSize / (1024 * 1024) << " MB\n"
       << "System Cache: " << pi.SystemCache * pi.PageSize / (1024 * 1024) << " MB\n";
    return ss.str();
}

std::string MemMonitor::getPageFileInfo() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (!GlobalMemoryStatusEx(&memInfo)) {
        return "Failed to get memory status\n";
    }
    
    std::stringstream ss;
    ss << "Page File Information:\n"
       << "Total Page File: " << memInfo.ullTotalPageFile / (1024 * 1024) << " MB\n"
       << "Available Page File: " << memInfo.ullAvailPageFile / (1024 * 1024) << " MB\n"
       << "Page File Usage: " 
       << std::fixed << std::setprecision(2)
       << (100.0f * (memInfo.ullTotalPageFile - memInfo.ullAvailPageFile) / 
           memInfo.ullTotalPageFile)
       << "%\n";
    return ss.str();
}




