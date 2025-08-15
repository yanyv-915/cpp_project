#include<thread>
#include<vector>
#include<queue>
#include<condition_variable>
#include<functional>
#include<atomic>
#include<mutex>
#include<iostream>
using namespace std;
class ThreadPool{
private:
    vector<thread>workers;    
    queue<function<void()>>tasks;
    mutex queue_mutex;
    condition_variable condition;
    atomic<bool>stop;
public:
    ThreadPool(size_t thread_count=thread::hardware_concurrency()):stop(false){
        for(size_t i=0;i<thread_count;i++){
            workers.emplace_back([this](){
                while(true){
                    function<void()>task;
                    {
                        unique_lock<mutex> lock(queue_mutex);
                        condition.wait(lock,[this](){
                            return stop||!tasks.empty();
                        });
                        if(stop&&tasks.empty()) return;
                        task=move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<typename F>
    void enquene(F&& f){
        {
            lock_guard<mutex>lock(queue_mutex);
            tasks.emplace(forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool(){
        {
            lock_guard<mutex>lock(queue_mutex);
            stop=true;
        }
        condition.notify_all();
        for(auto &worker:workers){
            if(worker.joinable()) worker.join();
        }
    }
};
int main(){
    ThreadPool t;
    for(int i=0;i<4;i++){
        t.enquene([i](){
            printf("task %d is working\n",i);
        });
    }
    return 0;
}