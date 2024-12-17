#include <iostream>
#include <string>

#include "patterns/observer.hpp"
#include "patterns/chain.hpp"
#include "patterns/command.hpp"
#include "patterns/strategy.hpp"
#include "patterns/state.hpp"
#include "patterns/adapter.hpp"
#include "patterns/facade.hpp"
#include "patterns/composite.hpp"
#include "patterns/observer.hpp"
#include "modern/testnewfeature.hpp"
#include "threads/thread.hpp"
#include "io/asio.hpp"
#include "threads/fiber.hpp"
#include "threads/signalslot.hpp"

int main() 
{
    testSignalSlot();
    return 0;
}   