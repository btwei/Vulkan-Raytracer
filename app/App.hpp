#ifndef APP_HPP
#define APP_HPP

#include "Engine.hpp"

/**
 * @class App
 * @brief This is the top level class for my raytracing app
 */
class App {
public:
    App(int argc, char* argv[]);
    void run();
private:
    int _argc;
    char** _argv;

    vkrt::Engine _engine;
};

#endif