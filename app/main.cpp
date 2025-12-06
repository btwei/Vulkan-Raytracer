#include <iostream>
#include <stdexcept>

#include "App.hpp"

int main(int argc, char* argv[]) {
    try {
        App app(argc, argv);
        app.run();
    } catch (const std::exception &e) {
        // Handles fatal runtime errors, largely device incompatibility or some unexpected interaction
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}