#include <iostream>
#include "webserver.h"

int main(int, char**){
    WebServer s(1234);
    s.init();
    s.eventLoop();
    std::cout << "Hello, from LrnWebServer!\n";
}
