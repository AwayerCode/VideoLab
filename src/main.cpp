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
#include "io/asio.hpp"
#include "threads/async.hpp"
#include "utils/timezone.hpp"
#include "template/template.hpp"
#include "ffmpeg/ffmpeg.hpp"

int main() 
{
    FFmpeg ffmpeg;
    auto ret = ffmpeg.openFile("test.mp4");
    if (!ret) {
        std::cerr << "open file failed" << std::endl;
        return -1;
    }
    std::cout << "open file success" << std::endl;

    while (ffmpeg.readFrame()) {
        std::cout << "read frame" << std::endl;
    }
    ffmpeg.closeFile();
    
    return 0;
}   