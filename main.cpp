#include <iostream>
#include <shared_mutex>
#include <fstream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
using namespace std;

unordered_map<string, streampos> index_;
shared_mutex dbMutex;

void buildIndex(){
    unique_lock<shared_mutex> lock(dbMutex);
    index_.clear();
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
    //cout<<"Thread "<<this_thread::get_id()<<" is setting "<<key<<" = "<<value<<endl;
    unique_lock<shared_mutex> lock(dbMutex);
    //this_thread::sleep_for(chrono::milliseconds(100));

    ofstream wal("wal.log");
    wal<<"SET "<<key<<" "<<value<<endl;
    wal.close();

    ofstream file("data.db", ios::app);
    if(!file.is_open()){
        cout<<"Error: could not open data.db"<<endl;
        return;
    }
    streampos pos=file.tellp();
    file<<key<<"="<<value<<endl;
    file.close();
    index_[key]=pos;

    wal.open("wal.log");
    wal.close();

    if(value=="__Deleted__"){
        cout<<"Deleted: "<<key<<endl;
    }
    else{
        cout<<"Saved: "<<key<<" = "<<value<<endl;
    }
}
void get(string key){
    shared_lock<shared_mutex> lock(dbMutex);

    ifstream file("data.db");
    string line;
    if(!file.is_open()){
        cout<<"Error: could not open data.db"<<endl;
        return;
    }

    if(index_.count(key)==0){
        cout<<"Not found: "<<key<<endl;
        return;
    }
    else{
        streampos pos=index_[key];
        file.seekg(pos);
    }
    getline(file, line);
    int pos2 = line.find("=");
    string v=line.substr(pos2 +1);
    file.close();
    if(v=="__Deleted__"){
        cout<<"Not found: "<<key<<endl;
        return;
    }
    cout<<"Found: "<<key<<" = "<<v<<endl;
}
void deleteKey(string key){
    unique_lock<shared_mutex> lock(dbMutex);

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
        lock.unlock();
        set(key,"__Deleted__");
    }
    else{
        cout<<"Not found: "<<key<<endl;
    }
}
void compact(){
    unique_lock<shared_mutex> lock(dbMutex);
    ifstream file("data.db");
    ofstream temp("temp.db",ios::app);
    string line;
    if(!file.is_open()){
        cout<<"Error: could not open data.db"<<endl;
        return;
    }
    for(auto& pair : index_) { 
        string key = pair.first; 
        streampos pos = pair.second;
        file.seekg(pos);
        getline(file, line);
        int pos2 = line.find("=");
        string v=line.substr(pos2 +1);
        if(v!="__Deleted__"){
            temp<<key<<"="<<v<<endl;
        }
    }
    file.close();
    temp.close();
    remove("data.db");
    rename("temp.db", "data.db");
    
    index_.clear();
    ifstream file2("data.db");
    streampos pos2=file2.tellg();
    while(getline(file2, line)){
        int eq = line.find("=");
        string k = line.substr(0, eq);
        index_[k] = pos2;
        pos2=file2.tellg();
    }
    file2.close();
}

void recoverFromWAL(){
    ifstream wal("wal.log");
    string line;
    if(!wal.is_open()){
        cout<<"Error: could not open wal.log"<<endl;
        return;
    }
    getline(wal, line);
    wal.close();
    if(line.empty()){
        return;
    }
    string restOfLine = line.substr(4);
    int pos = restOfLine.find(" ");
    string k = restOfLine.substr(0, pos);
    string v = restOfLine.substr(pos + 1);
    set(k, v);
}

int main(){
    recoverFromWAL();
    buildIndex();

    vector<thread> threads;

    threads.push_back(thread(set, "key1", "value1"));
    threads.push_back(thread(set, "key2", "value2"));
    threads.push_back(thread(set, "key3", "value3"));
    threads.push_back(thread(set, "key4", "value4"));
    threads.push_back(thread(set, "key5", "value5"));

    for(auto& t : threads){
        t.join();
    }
    cout<<"All threads completed."<<endl;
    get("key1");
    get("key2");    
    get("key3");
    get("key4");
    get("key5");

    return 0;
}