#ifndef APP_HPP
#define APP_HPP

/**
 * @class App
 * @brief This is the top level class for the demo app
 */
class App {
public:
    App(int argc, char* argv[]);
    void run();
private:
    int _argc;
    char** _argv;
};

#endif // APP_HPP