#include <iostream>
#include "engine.h"

int main(int argc, char* argv[]) {
    Engine engine;
    engine.init();
    engine.run();
    std::cout << "Cleaning up...\n";
    engine.cleanup();
    std::cout << "Goodbye!\n";
    return 0;
}
