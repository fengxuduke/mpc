#include <iostream>
#include "communicator/Writer.h"
#include "utils/stat.h"
//#include "utils.h"
using std::cout;
using std::endl;

int main() {
    cpu_set_affinity(2);
    auto writer=Writer::create("/tmp/trading/testjournal","testj5");
    int i=-1;
    writer->WriteFrame(static_cast<void *>(&i), sizeof(int)); 
    long long duration = getNanoTime()-(writer->frame).getNano();
    std::cout<<"time duration:"<<duration <<std::endl;   
    for(int i=0;i<=100;) {
        
        if (duration>10000000000){
           writer->WriteFrame(static_cast<void *>(&i), sizeof(int));
           duration=getNanoTime()-(writer->frame).getNano();
           std::cout<<"time duration:"<<duration <<std::endl;
           ++i;   
	}
        else
           duration=getNanoTime()-(writer->frame).getNano();
        
        
    }

    return 0;
}
