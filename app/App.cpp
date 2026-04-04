#include "App.hpp"

#include "Engine.hpp"

App::App(int argc, char* argv[]) 
: _argc(argc)
, _argv(argv) { }

void App::run() {
    vke::Engine e;

    // Considering a while loop here instead of a run() function in my previous engine designs
}