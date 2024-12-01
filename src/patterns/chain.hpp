#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

class FilterBase {
public:
    virtual ~FilterBase() = default;

    virtual bool process(std::shared_ptr<std::string> data) = 0;
    virtual void setType(const std::string& type) = 0;
};

class Filter : public FilterBase {
public:
    Filter(const std::string& name) : name(name) {}
    Filter(const std::string& name, const std::string& type) : name(name), type(type) {}
    ~Filter() = default;

    bool process(std::shared_ptr<std::string> data) override
    {
        if (data == nullptr) {
            return false;
        }

        if (data->find(type) == std::string::npos) {
            data->append("+" + type);
        }

        std::cout << name << " processing data: " << *data << std::endl;

        return true;
    }

    void setType(const std::string& type) override
    {
        this->type = type;
    }

private:
    std::string name;
    std::string type;
};

class FilterChain {
public:
    FilterChain() = default;
    ~FilterChain() = default;

    void addFilter(std::shared_ptr<FilterBase> filter)
    {
        filters.push_back(filter);
    }

    bool process(std::shared_ptr<std::string> data)
    {
        for (auto& filter : filters) {
            if (!filter->process(data)) {
                return false;
            }
        }

        return true;
    }

private:
    std::vector<std::shared_ptr<FilterBase>> filters;
};

void testChain()
{
    auto filter1 = std::make_shared<Filter>("Filter 1", "A");
    auto filter2 = std::make_shared<Filter>("Filter 2", "B");
    auto filter3 = std::make_shared<Filter>("Filter 3", "C");

    FilterChain chain;
    chain.addFilter(filter1);
    chain.addFilter(filter2);
    chain.addFilter(filter3);

    chain.process(std::make_shared<std::string>("Hello, world!"));
}
