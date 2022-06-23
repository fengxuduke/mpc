#include <iostream>
#include "communicator/Reader.h"
#include "utils/stat.h"
//#include "utils.h"
using std::cout;
using std::endl;

int main() {
    cpu_set_affinity(1);
    auto reader=Reader::create("testReader");
    std::cout<<"before addJournal"<<std::endl;
    reader->addJournal("/home/feng/Documents/trading/testjournal","testj5");
    std::cout<<"after addJournal"<<std::endl;
    while(true){
        // auto frameptr= static_cast<Frame *> (reader->readFrame());
        void* frameptr = reader->readFrame();
        if(frameptr== nullptr)
            continue;
        else{
        auto frame = Frame(frameptr);
        long duration=getNanoTime()-frame.getNano();
        std::cout<<"time duration:"<<duration <<std::endl;
        }
    }

    return 0;
}
