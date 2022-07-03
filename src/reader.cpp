#include <iostream>
#include "communicator/Reader.h"
#include "utils/stat.h"
#include "ctp/ThostFtdcUserApiStruct.h"
//#include "utils.h"
using std::cout;
using std::endl;

int main() {
    cpu_set_affinity(1);
    auto reader=Reader::create("testReader");
    std::cout<<"before addJournal"<<std::endl;
    reader->addJournal("/tmp/trading/testjournal","testj5");
    std::cout<<"after addJournal"<<std::endl;
    while(true){
        // auto frameptr= static_cast<Frame *> (reader->readFrame());
        void* frameptr = reader->readFrame();
        if(frameptr== nullptr)
            continue;
        else{
        auto frame = Frame(frameptr);
        long duration=getNanoTime()-frame.getNano();
        auto data = static_cast<CThostFtdcDepthMarketDataField*> (frame.getData() );
        std::cout<<"time duration:"<<duration<<" "<<data->LastPrice<<std::endl;
        }
    }

    return 0;
}
