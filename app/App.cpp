#include "App.hpp"

App::App(int argc, char* argv[]) :
    _argc(argc),
    _argv(argv){ }

void App::run() {
    init();
    mainLoop();
    cleanup();
}

void App::init() {
    
}

void App::mainLoop() {

}

void App::cleanup() {

}