#include"client.h"
using namespace std;
int main(){
    Config config("config.json");
    Client client(config.cfg.client);
    client.run();
}