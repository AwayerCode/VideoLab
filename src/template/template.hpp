#pragma once

#include <iostream>

template <typename T>
void print(T value) {
    std::cout << value << std::endl;
}

template <typename T>
class TemplateClass {
public:
    TemplateClass(T value) : value(value) {}
    void print() {
        std::cout << value << std::endl;
    }
private:
    T value;
};

void testTemplate() {
    print(1);

    TemplateClass<int> intClass(1);
    intClass.print();
}