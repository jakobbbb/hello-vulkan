#define APP_NAME "Hewwo :3"
#include "engine.h"

int main(int argc, char* argv[]) {
    Engine engine;
    engine.init();
    engine.run();
    engine.cleanup();
    return 0;
}
