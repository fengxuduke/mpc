#include <iostream>
#include "communicator/Writer.h"
#include "utils/stat.h"
//#include "utils.h"
using std::cout;
using std::endl;

int main() {
    cpu_set_affinity(2);
    auto writer=Writer::create("/tmp/trading/testjournal","testj5");
    for(int i=0;i<=100;++i) {
        usleep(10000000);
        writer->WriteFrame(static_cast<void *>(&i), sizeof(int));
    }

    return 0;
}
