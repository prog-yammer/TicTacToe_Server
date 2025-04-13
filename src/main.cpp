#include "web/server.h"

int main()
{
    Server server(std::thread::hardware_concurrency(), 8080);
    server.start();

    return 0;
}