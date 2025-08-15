#include<mutex>
#include<condition_variable>
#include<vector>
#include<thread>
#include<queue>
#include<atomic>
#include<functional>
using namespace std;
class ThreadPool{
private:
    vector<thread>workers;
    queue<function<void()>>tasks;
    mutex mtx;
    condition_variable condition;
    atomic<bool>stop;
public:
    ThreadPool(ssize_t thread_size=thread::hardware_concurrency()):stop(false){
        for(int i=0;i<thread_size;i++){
            workers.emplace_back([this](){
                while(true){
                    function<void()>task;
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
        {
            unique_lock<mutex> lock(mtx);
            stop=true;
        }
        condition.notify_all();
        for(auto& t:workers){
            if(t.joinable()) t.join();
        }
    }
};