#pragma once

#include <string_view>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <vector>
#include <algorithm>
#include <map>
#include <variant>
#include <any>

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

// optional：更安全的空值处理
// 哪怕是UserProfile这种复杂场景，依然可以通过给UserProfile增加一个isEmpty的方法来判断，简单明了。
// 为什么要引入新的option特性。直接用数据+函数的方式，更加直观，就像自然语言一样。
// 你说得对，这样的好处是：
// 代码更像自然语言，易于理解
// 不需要引入新概念
// 直接用业务语义表达意图
// 更容易维护和扩展
// 确实没必要为了用新特性而强行使用 optional！
void testOptional()
{
    std::optional<int> opt = 1;
    std::cout << *opt << std::endl;
}

// variant：类型安全的多态
// variant还有用，在状态机的设置和返回值优化上可以提高
// variant 的优势在于：
// 状态机：类型安全的状态转换
// 返回值：可以返回多种类型，比多个返回值更清晰
// 编译时类型检查，避免运行时错误
void testVariant()
{
    std::variant<int, double> var = 1;
    std::cout << std::get<int>(var) << std::endl;
}

// any：类型安全的容器
// std::any 确实和 void* 类似，但比 void* 更安全一些
// 可以作为key value的value存储
void testAny()
{
    std::any any = 1;
    std::cout << std::any_cast<int>(any) << std::endl;
}

void testNewFeature()
{
    testOptional();
    testVariant();
    testAny();
}