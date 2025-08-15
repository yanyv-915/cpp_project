#include<iostream>
#include<vector>
#include<algorithm>
using namespace std;
int main(){
    //[]不捕获任何变量
    auto sayhello=[](){cout<<"say hello\n";};
    sayhello();
    int x=10,y=100;
    //[x]捕获拷贝外部变量x的值
    auto byValue=[x](){cout<<x<<endl;};
    ////[&y]对外部变量y进行引用
    auto byRef=[&y](){y+=5;};
    byValue();
    byRef();
    cout<<"y = "<<y<<endl;
    cout<<"-----------------------\n";
    vector<int>a={1,5,2,4,3};
    sort(a.begin(),a.end(),[](int a,int b){
        return a>b;
    });
    for_each(a.begin(),a.end(),[](int x){
        cout<<x<<" ";
    });
    cout<<endl;

    vector<string>v={"eunuwwcv","uincece","usnianeedbwub"};
    //使用const string&常量引用,避免不必要的拷贝浪费资源
    sort(v.begin(),v.end(),[](const string& a,const string& b){
        return a.size()<b.size();
    });
    for(auto x:v) cout<<x<<" ";
    cout<<endl;
    for_each(a.begin(),a.end(),[](int&x){
        x*=2;
    });
    for(auto x:a) cout<<x<<" ";
    cout<<endl;
}