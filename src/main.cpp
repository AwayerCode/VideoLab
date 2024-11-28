#include <iostream>
#include <string>
#include "FileInfo/FileInfo.h"
#include "GlobalRoute.h"

int main() {
    std::string input;
    
    while (true) {
        std::cout << "Enter command (type 'quit' to exit): ";
        std::getline(std::cin, input);
        
        if (input == "quit") {
            break;
        }

        GlobalRoute::getInstance().processCommand(input);
        
        std::cout << "You entered: " << input << std::endl;
    }
    
    std::cout << "Program terminated." << std::endl;
    return 0;
}   