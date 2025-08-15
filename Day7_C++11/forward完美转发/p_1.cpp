#include <iostream>
#include <utility> // std::forward

using namespace std;

// 先声明（或定义）callee
void callee(const int& x) { cout << "左值版本\n"; }
void callee(int&& x) { cout << "右值版本\n"; }

template<typename T>
void wrapper(T&& arg) {
    callee(std::forward<T>(arg)); // 保留原来的左值/右值特性
}

int main() {
    int a = 42;
    wrapper(a);    // 输出 左值版本
    wrapper(100);  // 输出 右值版本
}
