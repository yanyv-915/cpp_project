#include <iostream>
#include <vector>
#include <utility>
using namespace std;

struct Task {
    Task() { cout << "默认构造\n"; }
    Task(const Task&) { cout << "拷贝构造\n"; }
    Task(Task&&) noexcept { cout << "移动构造\n"; }
};
vector<Task> queue1;
vector<Task> queue2;

// 不用 forward
template<typename F>
void enqueue_no_forward(F&& f) {
    cout << "[enqueue_no_forward]\n";
    queue1.emplace_back(f); // 这里 f 永远是左值 → 会调用拷贝构造
}

// 用 forward
template<typename F>
void enqueue_with_forward(F&& f) {
    cout << "[enqueue_with_forward]\n";
    queue2.emplace_back(std::forward<F>(f)); // 保留原来的值类别
}

int main() {
    cout << "=== 传右值 Task() ===\n";
    enqueue_no_forward(Task());    // 右值任务 → 被当作左值拷贝
    enqueue_with_forward(Task());  // 右值任务 → 直接移动

    cout << "\n=== 传左值 Task ===\n";
    Task t;
    enqueue_no_forward(t);         // 左值 → 拷贝
    cout<<endl;
    enqueue_with_forward(t);       // 左值 → 拷贝（与右值不同）
}
