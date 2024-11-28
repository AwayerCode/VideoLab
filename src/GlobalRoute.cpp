#include "GlobalRoute.h"
#include <iostream>

GlobalRoute& GlobalRoute::getInstance() {
    static GlobalRoute instance;
    return instance;
}

void GlobalRoute::processCommand(const std::string& command) {
    std::cout << "Processing command: " << command << std::endl;
}
