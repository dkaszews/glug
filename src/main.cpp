#include <iostream>
#include <tuple>

int main(int argc, char** argv) {
    std::ignore = std::tie(argc, argv);
    std::cout << "Hello glug!" << std::endl;
}

