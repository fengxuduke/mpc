#include <iostream>
#include <fstream>
#include "communicator/Reader.h"
#include "utils/stat.h"
#include <signal.h>
#include "ctpconfig.h"
//#include "utils.h"
using std::cout;
using std::endl;

long long TickProcessDurations[40000*72];
TThostFtdcInstrumentIDType TickedInstrumentIDs[40000*72];
long long TickedTimes[40000*72];
const string dumpfilepath_="/home/feng/mpc-yijinjing-mmap/ctp-run-statistics/ctp_read_stats.csv";
int tick_idx = -1;
void dumptofile(  ){
    int i=0;
    std::ofstream fout;
    fout.open(dumpfilepath_.c_str());
    fout<<"arrivaltime,"<<"ticker,"<<"latency"<<std::endl;
    for(i=0;i<=tick_idx;i++){
        fout<<TickedTimes[i]<<","<<TickedInstrumentIDs[i]<<","<<TickProcessDurations[i]<<std::endl;
    }
};

void signal_callback_handler(int signum) {
   std::cout << "Caught signal " << signum << std::endl;
   // Terminate program
   dumptofile();
   exit(signum);
};

int main() {
    cpu_set_affinity(1);
    signal(SIGINT, signal_callback_handler);
//     auto reader=Reader::create("testReader");
    ReaderPtr readers[72];
    ReaderPtr reader;
    std::cout<<"before addJournal"<<std::endl;
    int month=0;
    int reader_idx=0;
    std::vector<int> months={7,8,9,12};
    TThostFtdcInstrumentIDType instrument;
    for (auto& month:months){
        sprintf(instrument, "IC22%02d", month);
        reader = Reader::create(instrument);
        readers[reader_idx++]=reader;
        reader->addJournal("/tmp/trading/testjournal", instrument);
        
        sprintf(instrument, "IF22%02d", month);
        reader = Reader::create(instrument);
        readers[reader_idx++]=reader;
        reader->addJournal("/tmp/trading/testjournal", instrument);
        
        sprintf(instrument, "IH22%02d", month);
        reader = Reader::create(instrument);
        readers[reader_idx++]=reader;
        reader->addJournal("/tmp/trading/testjournal", instrument);
    }
    
    std::cout<<"after addJournal"<<std::endl;
    while(true){
        // auto frameptr= static_cast<Frame *> (reader->readFrame());
        for (reader_idx=0;reader_idx<months.size()*3;reader_idx++){
            auto& reader = readers[reader_idx];
            void* frameptr = reader->readFrame();
            if(frameptr== nullptr)
                continue;
            else{
                auto frame = Frame(frameptr);
    //             long duration=getNanoTime()-frame.getNano();
                auto data = static_cast<CThostFtdcDepthMarketDataField*> (frame.getData() );
                ++tick_idx;            
                TickedTimes[tick_idx] = frame.getNano();            
                mempcpy(TickedInstrumentIDs[tick_idx], data->InstrumentID, sizeof(TThostFtdcInstrumentIDType));
                TickedInstrumentIDs[tick_idx][sizeof(TThostFtdcInstrumentIDType)-1]='\0';
                TickProcessDurations[tick_idx] = getNanoTime() - TickedTimes[tick_idx];
            }
        }
    }

    return 0;
}
