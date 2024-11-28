#include <iostream>
#include <string>

int main() {
    std::string input;
    
    while (true) {
        std::cout << "请输入命令 (输入 'quit' 退出): ";
        std::getline(std::cin, input);
        
        if (input == "quit") {
            break;
        }
        
        // 处理输入的命令
        std::cout << "您输入的是: " << input << std::endl;
    }
    
    std::cout << "程序已退出" << std::endl;
    return 0;
} 