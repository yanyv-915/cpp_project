#include<iostream>
#include<utility>
#include<string>
using namespace std;
struct MyData{
public:
    MyData(string s):name(move(s)){
        cout<<"构造 "<<name<<endl;
    }
    MyData(const MyData&other){
        name=other.name;
        cout<<"拷贝构造 "<<name<<endl;
    }
    MyData(const MyData&& other) noexcept{
        name=move(other.name);
        cout<<"移动构造 "<<name<<endl;
    }
    ~MyData(){
        cout<<"析构 "<<name<<endl;
    }
    string name;
};
void normalPass(MyData data){
    cout<<"normalPass收到: "<<data.name<<endl;
}

template<typename T>
void perfectForward(T&& data){
    cout<<"perfectForward收到: "<<data.name<<endl;
    MyData obj=forward<T>(data);
}


int main(){
    cout << "=== 1. 直接构造 ===" << endl;
    MyData a("Hello");

    cout << "\n=== 2. 值传递 ===" << endl;
    normalPass(a); // a是左值 → 拷贝

    cout << "\n=== 3. 值传递右值 ===" << endl;
    normalPass(MyData("Temp")); // 临时对象 → 移动（可能优化）

    cout << "\n=== 4. 完美转发 左值 ===" << endl;
    perfectForward(a); // 左值 → 拷贝

    cout << "\n=== 5. 完美转发 右值 ===" << endl;
    perfectForward(MyData("World")); // 右值 → 移动

    cout << "\n=== 结束 ===" << endl;
    
}