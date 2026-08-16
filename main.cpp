#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
using namespace std;

unordered_map<string, streampos> index_;

void buildIndex(){
    ifstream file("data.db");
    string line;
    if(!file.is_open()){
        cout<<"Error: could not open data.db"<<endl;
        return;
    }
    streampos pos=file.tellg();
    while(getline(file, line)){
        int eq = line.find("=");
        string k = line.substr(0, eq);
        index_[k] = pos;
        pos=file.tellg();
    }
    file.close();
}
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

    bool found = false;
    string lastValue;

    while(getline(file, line)){
        int pos = line.find("=");
        string k = line.substr(0, pos);
        string v = line.substr(pos + 1);
        if(k == key){
            found = true;
            lastValue = v;  
        }
    }
    file.close();

    if(found && lastValue != "__Deleted__"){
        cout<<"Found: "<<key<<" = "<<lastValue<<endl;
    } else {
        cout<<"Not found: "<<key<<endl;
    }
}

void deleteKey(string key){
    ifstream file("data.db");
    string line;
    bool found=false;
    if(!file.is_open()){
        cout<<"Error: could not open data.db"<<endl;
        return;
    }
    while(getline(file, line)){
        int pos = line.find("=");
        string k = line.substr(0, pos);
        if(k == key){
            found=true;
        }
    }
    file.close();
    if(found){
        set(key,"__Deleted__");
    }
    else{
        cout<<"Not found: "<<key<<endl;
    }
}

int main(){
    set("username","rahul123");
    set("age","20");

    buildIndex();

    cout << "Index for username: " << index_["username"] << endl;
    cout << "Index for age: " << index_["age"] << endl;

    get("username");
    get("age");
    deleteKey("age");
    get("age");

    return 0;
}