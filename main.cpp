#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
using namespace std;

unordered_map<string, streampos> index_;

void buildIndex(){
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

    ofstream file("data.db", ios::app);

    if(!file.is_open()){
        cout<<"Error: could not open data.db"<<endl;
        return;
    }
    streampos pos=file.tellp();
    file<<key<<"="<<value<<endl;
    file.close();
    index_[key]=pos;
    if(value=="__Deleted__"){
        cout<<"Deleted: "<<key<<endl;
    }
    else{
        cout<<"Saved: "<<key<<" = "<<value<<endl;
    }
}

void get(string key){
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

void compact(){
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
    buildIndex();
}

int main(){
    buildIndex();   

    set("username", "rahul123");
    set("age", "20");
    set("age", "21");        

    get("username");
    get("age");

    deleteKey("username");
    get("username");         

    compact();               

    get("age");               

    return 0;
}