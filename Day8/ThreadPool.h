#include<thread>
#include<vector>
#include<queue>
#include<functional>
#include<condition_variable>
#include<mutex>
#include<atomic>
using namespace std;

class ThreadPool{

private:
    mutex mtx;
    vector<thread>workers;
    queue<function<void()>> tasks;
    condition_variable condition;
    atomic<bool> stop;

public:
    ThreadPool(ssize_t thread_num=thread::hardware_concurrency()):stop(false){
        for(ssize_t i=0;i<thread_num;i++){
            workers.emplace_back([this](){
                while(true){
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(mtx);
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
    void enqueue(F&& f){
        {
            unique_lock<mutex> lock(mtx);
            tasks.emplace(forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool(){
        stop=false;
        condition.notify_all();
        for(thread &worker:workers){
            if(worker.joinable()) worker.join();
        }
    }
};