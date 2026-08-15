#include <iostream>
#include <fstream>
#include <string>
using namespace std;

void set(string key, string value){
    ofstream file("data.db", ios::app);

    if(!file.is_open()){
        cout<<"Error: could not open data.db"<<endl;
        return;
    }
    file<<key<<"="<<value<<endl;
    file.close();
    cout<<"Saved: "<<key<<" = "<<value<<endl;
}

void get(string key){
    ifstream file("data.db");
    string line;
    if(!file.is_open()){
        cout<<"Error: could not open data.db"<<endl;
        return;
    }
    while(getline(file, line)){
        int pos = line.find("=");
        string k = line.substr(0, pos);
        string v = line.substr(pos + 1);
        if(k == key){
            cout<<"Found: "<<key<<" = "<<v<<endl;
            file.close();
            return;
        }
    }
    cout<<"Not found: "<<key<<endl;
    file.close();
}
int main(){
    set("username","rahul123");
    set("age","20");
    get("username");
    get("age");
    return 0;
}