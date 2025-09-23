#include"MyHeader.h"
using namespace std;
int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);

    connect(sock, (sockaddr*)&serv, sizeof(serv));

    // 连续发送三条消息
    send(sock, "Hello", 5, 0);
    send(sock, "World", 5, 0);
    send(sock, "!!!", 3, 0);

    close(sock);
    return 0;
}