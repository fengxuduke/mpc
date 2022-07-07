//
// Created by llx on 2018/8/13.
//
#include "MpcServer.h"
#include "../utils/stat.h"
#include<iostream>
using std::cout;
using std::endl;
int main(){
    cpu_set_affinity(0);
    auto service=MpcServer::getInstance();
   // service->run();
    cout<<"after create instance"<<endl;
    service->Start();
    service->run();

}