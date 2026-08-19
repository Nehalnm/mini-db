#include <iostream>
#include <shared_mutex>
#include <fstream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
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
string set(string key, string value){
    unique_lock<shared_mutex> lock(dbMutex);

    ofstream wal("wal.log");
    wal<<"SET "<<key<<" "<<value<<endl;
    wal.close();

    ofstream file("data.db", ios::app);
    if(!file.is_open()){
        return "Error: could not open data.db";
    }
    streampos pos=file.tellp();
    file<<key<<"="<<value<<endl;
    file.close();
    index_[key]=pos;

    wal.open("wal.log");
    wal.close();

    if(value=="__Deleted__"){
        return "Deleted: " + key;
    }
    else{
        return "Saved: " + key + " = " + value;
    }
}
string get(string key){
    shared_lock<shared_mutex> lock(dbMutex);

    ifstream file("data.db");
    string line;
    if(!file.is_open()){
        return "Error: could not open data.db";
    }

    if(index_.count(key)==0){
        return "Not found: " + key;
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
        return "Not found: " + key;
    }
    return "Found: " + key + " = " + v;
}
string deleteKey(string key){
    unique_lock<shared_mutex> lock(dbMutex);

    ifstream file("data.db");
    string line;
    bool found=false;
    if(!file.is_open()){
        return "Error: could not open data.db";
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
        return set(key,"__Deleted__");
    }
    else{
        return "Not found: " + key;
    }
}
void compact(){
    unique_lock<shared_mutex> lock(dbMutex);
    ifstream file("data.db");
    ofstream temp("temp.db",ios::app);
    string line;
    if(!file.is_open()){
        cout<<"No data.db found - starting fresh"<<endl;
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
        cout<<"No wal.log found - starting fresh"<<endl;
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
void handleClient(int clientSocket){
    char buffer[1024] = {0};
        read(clientSocket, buffer, 1024);
        string command(buffer);

        if(!command.empty() && command[command.length() - 1] == '\n'){
            command = command.substr(0, command.length() - 1);
        }

        string response;

        if(command.substr(0, 4) == "GET "){
            string key = command.substr(4);
            response = get(key);
        }
        else if(command.substr(0, 4) == "SET "){
            string rest = command.substr(4);
            int spacePos = rest.find(" ");
            string key = rest.substr(0, spacePos);
            string value = rest.substr(spacePos + 1);
            response = set(key, value);
        }
        else if(command.substr(0, 7) == "DELETE "){
            string key = command.substr(7);
            response = deleteKey(key);
        }
        else{
            response = "Unknown command";
        }

        response += "\n";
        send(clientSocket, response.c_str(), response.length(), 0);
        close(clientSocket);
}
void runServer(){
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1){
        cout << "Error: could not create socket" << endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(9999);

    if(bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
        cout << "Error: bind failed" << endl;
        return;
    }

    listen(serverSocket, 5);
    cout << "Server listening on port 9999..." << endl;

    while(true){
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if(clientSocket < 0){
            continue;
        }
        thread(handleClient, clientSocket).detach();
    }
}
void autoCompact(){
    while(true){
        this_thread::sleep_for(chrono::seconds(300)); 
        compact();
        cout << "Auto-compaction ran." << endl;
    }
}
int main(){
    recoverFromWAL();
    buildIndex();

    thread(autoCompact).detach();

    runServer();

    return 0;
}