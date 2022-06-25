#include <iostream>
#include "communicator/Writer.h"
#include "utils/stat.h"
#include "ctp/ThostFtdcUserApiStruct.h"
//#include "utils.h"
using std::cout;
using std::endl;

int main() {
    cpu_set_affinity(2);
    auto writer=Writer::create("/tmp/trading/testjournal","testj5");
    int i=-1;
    CThostFtdcDepthMarketDataField datas[102];
    size_t length = sizeof(CThostFtdcDepthMarketDataField);
    std::cout<<"data size="<<length<<std::endl;
    datas[0].LastPrice=i;
    writer->WriteFrame(static_cast<void *>(&datas[0]), length); 
    long long duration = getNanoTime()-(writer->frame).getNano();
    std::cout<<"time duration:"<<duration <<std::endl;   
    for(int i=0;i<=100;) {
        
        if (duration>125000000){
           datas[i+1].LastPrice=i;
    	   writer->WriteFrame(static_cast<void *>(&datas[i+1]), length);
           duration=getNanoTime()-(writer->frame).getNano();
           std::cout<<"time duration:"<<duration <<std::endl;
           ++i;   
	}
        else
           duration=getNanoTime()-(writer->frame).getNano();
        
        
    }

    return 0;
}
