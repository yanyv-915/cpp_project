#include"server.h"
using namespace std;
int main(){
    Config config("config.json");
    Server server(config.cfg.server);
    server.setUpServer();
}