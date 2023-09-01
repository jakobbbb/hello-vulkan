#include <iostream>
#include "glm/common.hpp"
#include "glm/gtx/norm.hpp"

int main() {
    glm::vec3 hello{1, 1, 0};
    float dist = glm::distance(glm::vec3{}, hello);
    std::cout << "Hello, " << dist << "!\n";
    return 0;
}
