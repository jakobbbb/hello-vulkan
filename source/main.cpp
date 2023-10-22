#include <iostream>
#include "hello_engine.h"

int main(int argc, char* argv[]) {
    HelloEngine engine;
    engine.init();
    engine.run();
    std::cout << "Cleaning up...\n";
    engine.cleanup();
    std::cout << "Goodbye!\n";
    return 0;
}
