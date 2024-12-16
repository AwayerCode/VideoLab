#pragma once

#include <iostream>
#include <string>
#include <memory>
class NetworkSystem {
public:
    void connect() {
        std::cout << "NetworkSystem: connecting to the network" << std::endl;
    }
};

class StorageSystem {
public:
    void store() {
        std::cout << "StorageSystem: storing data" << std::endl;
    }
};

class APIGateway {
public:
    APIGateway() {
        networkSys = std::make_shared<NetworkSystem>();
        storageSys = std::make_shared<StorageSystem>();
    }

    void connect() {
        networkSys->connect();
    }

    void store() {
        storageSys->store();
    }

private:
    std::shared_ptr<NetworkSystem> networkSys;
    std::shared_ptr<StorageSystem> storageSys;
};

void testFacade()
{
    
}