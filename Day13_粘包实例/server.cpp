#include<iostream>
#include"MyHeader.h"
using namespace std;
int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    cout << "Server listening on port " << PORT << endl;
    int client_fd = accept(server_fd, nullptr, nullptr);

    char buf[BUF_SIZE];
    while (true) {
        int n = recv(client_fd, buf, sizeof(buf)-1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        cout << "[Server recv] " << buf << endl;
    }

    close(client_fd);
    close(server_fd);
    return 0;
}