#include"config.h"
using namespace std;
int main(){
    try{
        Config config("config.json");
        cout<<config.cfg.client.PORT<<endl;
        cout<<config.cfg.server.HEART_BEAT<<endl;
    }
    catch (exception& e) {
        std::cerr << "配置读取失败: " << e.what() << std::endl;
    }
}