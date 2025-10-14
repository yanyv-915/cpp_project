#pragma once
#include<vector>
#include<queue>
#include<functional>
#include<mutex>
#include<condition_variable>
#include<thread>
#include<atomic>
using namespace std;
class ThreadPool{
private:
        vector<thread>works;
        queue<function<void()>>tasks;
        condition_variable cv;
        atomic<bool>stop;
        mutex mtx;
public:
        ThreadPool(int threads=thread::hardware_concurrency()):stop(false){
            for(int i=0;i<threads;i++){
                works.emplace_back([this](){
                    while(true){
                        function<void()> task;
                        {
                            unique_lock<mutex> lk(mtx);
                            cv.wait(lk, [this]()
                                    { return stop || !tasks.empty(); });
                            if (stop && tasks.empty())
                            {
                                return;
                            }
                            task = move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                });
            }
        }
        template<typename F>
        void enqueue(F &&f){
            {
                unique_lock<mutex> lk(mtx);
                tasks.emplace(forward<F>(f));
            }
            cv.notify_one();
        }

        ~ThreadPool(){
            {
                unique_lock<mutex> lk(mtx);
                stop=true;
                cv.notify_all();
            }
            for(thread &w:works){
                if(w.joinable()) w.join();
            }
        }
};