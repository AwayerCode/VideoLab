#pragma once

#include <string>

class Target {
public:
    virtual ~Target() = default;
    virtual std::string request() const = 0;
};