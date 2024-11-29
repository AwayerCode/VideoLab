#include "MemMonitor.h"

#include <windows.h>

MemMonitor::MemMonitor()
{

}

MemMonitor::~MemMonitor()
{
    
}

std::string MemMonitor::getMemOverview()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    DWORDLONG totalPhysMem = memInfo.ullTotalPhys; 
    DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
    DWORDLONG totalVirtualMem = memInfo.ullTotalVirtual; 

    std::string memOverviewResult = "MemOverview: \n";

    memOverviewResult += "Total Physical Memory: " + 
                            std::to_string(totalPhysMem / (1024 * 1024)) + " MB\n";
    memOverviewResult += "Used Physical Memory: " + 
                            std::to_string(physMemUsed / (1024 * 1024)) + " MB\n";
    memOverviewResult += "Total Virtual Memory: " + 
                            std::to_string(totalVirtualMem / (1024 * 1024)) + " MB";

    return memOverviewResult;
}




