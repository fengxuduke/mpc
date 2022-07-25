#include <iostream>
#include <fstream>
#include "communicator/Reader.h"
#include "utils/stat.h"
#include <signal.h>
#include "ctpconfig.h"
#include "common.h"
//#include "utils.h"
using std::cout;
using std::endl;

long long TickProcessDurations[40000*72];
TThostFtdcInstrumentIDType TickedInstrumentIDs[40000*72];
long long TickedTimes[40000*72];
TThostFtdcTimeType TickExchangeTimes[40000*72];
TThostFtdcMillisecType TickExchangeMillisecs[40000*72];
const string dumpfilepath_="/home/feng/mpc-yijinjing-mmap/ctp-run-statistics/ctp_read_stats.csv";
int tick_idx = -1;
void dumptofile(  ){
    int i=0;
    std::ofstream fout;
    fout.open(dumpfilepath_.c_str());
    fout<<"arrivaltime,"<<"exchangetime,"<<"exchangemillisec,"<<"ticker,"<<"latency"<<std::endl;
    for(i=0;i<=tick_idx;i++){
        fout<<TickedTimes[i]<<","<<TickExchangeTimes[i]<<","<<TickExchangeMillisecs[i]<<","<<TickedInstrumentIDs[i]<<","<<TickProcessDurations[i]<<std::endl;
    }
};

void signal_callback_handler(int signum) {
   std::cout << "Caught signal " << signum << std::endl;
   // Terminate program
   dumptofile();
   exit(signum);
};

void onDepthMarketData( void* pdata ){
    auto data = static_cast<CThostFtdcDepthMarketDataField*> (pdata );
    strcpy(TickedInstrumentIDs[tick_idx], data->InstrumentID);
    strcpy(TickExchangeTimes[tick_idx], data->UpdateTime);
    TickExchangeMillisecs[tick_idx] = data->UpdateMillisec;
    TickProcessDurations[tick_idx] = getNanoTime() - TickedTimes[tick_idx];
};

int main() {
    cpu_set_affinity(2);
    signal(SIGINT, signal_callback_handler);
//     auto reader=Reader::create("testReader");
    ReaderPtr readers[72];
    ReaderPtr reader;
    std::cout<<"before addJournal"<<std::endl;
    int month=0;
    int reader_idx=0;
    std::vector<int> months={7,8,9,12};
    std::vector<std::string> instruments={"SR209"};
    for (auto& instrument:instruments){
        reader = Reader::create(instrument.c_str());
        readers[reader_idx++]=reader;
        reader->addJournal("/tmp/trading/testjournal", instrument.c_str());
    }
    
    std::cout<<"after addJournal"<<std::endl;
    while(true){
        // auto frameptr= static_cast<Frame *> (reader->readFrame());
        for (reader_idx=0;reader_idx<instruments.size();reader_idx++){
            auto& reader = readers[reader_idx];
            void* frameptr = reader->readFrame();
            if(frameptr== nullptr)
                continue;
            else{
                auto frame = Frame(frameptr);
                auto msgtype = (MsgType)(frame.getMsgType());
    //             long duration=getNanoTime()-frame.getNano();
                switch(msgtype){
                    case MsgType::DepthMarketData:
                        tick_idx++;
                        TickedTimes[tick_idx] = frame.getNano();    
                        onDepthMarketData(frame.getData());
                        break;
                        
                }
                
                
            }
        }
    }

    return 0;
}
