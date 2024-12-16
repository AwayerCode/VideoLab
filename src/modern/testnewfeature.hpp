#pragma once

#include <string_view>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <vector>
#include <algorithm>
#include <map>

// string_view：提升字符串处理性能
// optional：更安全的空值处理
// variant：类型安全的多态
// ranges：简化算法使用
// 结构化绑定：简化代码
// 并行算法：性能提升
// filesystem：文件操作标准化
// make_unique：简化内存管理
// if switch：更简洁的条件判断

void testStringView()
{
    std::string_view sv = "Hello, world!";
    std::cout << sv << std::endl;
}

void testFilesystem()
{
    std::filesystem::path file_path = "test.txt"; 
    std::ofstream output(file_path);
    if(output) {
        output << "This is test file content" << std::endl;
        output.close();
        std::cout << "File created successfully: " << file_path << std::endl;
    } else {
        std::cout << "Failed to create file" << std::endl;
    }

    std::filesystem::path p = "test.txt";
    std::cout << p << std::endl;
}

void testRanges()
{
    std::vector<int> vec = {1, 3, 2, 5, 4};
    std::ranges::sort(vec);
    for(int i : vec) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
}


void testStructuredBinding()
{
    std::map<std::string, int> map = {{"a", 1}, {"b", 2}, {"c", 3}};
    for(const auto& [key, value] : map) {
        std::cout << key << " " << value << std::endl;
    }

    for(auto itr = map.begin(); itr != map.end(); ++itr) {
        std::cout << itr->first << " " << itr->second << std::endl;
    }
}

void testSmartPointer()
{
    auto ptr = std::make_unique<int>(1);
    auto sptr = std::make_shared<int>(2);
    std::cout << *ptr << std::endl;
    std::cout << *sptr << std::endl;
}



void testNewFeature()
{
    
}
