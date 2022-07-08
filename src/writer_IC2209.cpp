#include <iostream>
#include <fstream>
#include "communicator/Writer.h"
#include "utils/stat.h"
#include <signal.h>
#include "ctpconfig.h"
//#include "utils.h"
using std::cout;
using std::endl;


// CTPSUBSCRIBEPATTERNS = os.environ.get('CTPSUBSCRIBEPATTERNS','IF22%02d,IF23%02d,IH22%02d,IH23%02d,IC22%02d,IC23%02d')

class MarketDataCTP: public CThostFtdcMdSpi {

    public:
        MarketDataCTP(const char* dumpfilepath): dumpfilepath_(dumpfilepath){}; 
        void start(){
              papi_ = CThostFtdcMdApi::CreateFtdcMdApi();
              papi_->RegisterSpi(this);
              papi_->RegisterFront(CTPFRONTQUOTEADDRESS);
              papi_->Init();
//               papi_->Join();
        };
        
        void Join(){
            papi_->Join();
        }

        virtual ~MarketDataCTP()
        {};

        bool subscribe(const std::vector<string> &instruments){            
            char* ppInstruments[1];
            for (auto& instrument : instruments){
                ppInstruments[0]=(char*)&instrument[0];
                papi_->SubscribeMarketData(ppInstruments, 1);
                usleep(1000000);
            }
            return true;
        }; 


        bool unsubscribe(const std::vector<string> &instruments){
            char* ppInstruments[1];
            for (auto& instrument:instruments){
                ppInstruments[0]=(char*)&instrument[0];
                papi_->UnSubscribeMarketData(ppInstruments, 1);
                usleep(1000000);
            }
            return true;
        };

        bool login(){
            std::cout<<"to login"<<std::endl;
            CThostFtdcReqUserLoginField login_field = {};
            strcpy(login_field.UserID, CTPINVESTORID);
            strcpy(login_field.BrokerID, CTPBROKERID);
            strcpy(login_field.Password, CTPPASSWORD);

            int rtn = papi_->ReqUserLogin(&login_field, ++request_id_);
            std::cout<<"after req login: "<<rtn<<std::endl;
            return rtn == 0;
        };
        
        bool IsLoggedIn(){
            
            return IsLoggedIn_;
        };
        bool IsConnected(){
            return IsConnected_;
        };

        ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
        virtual void OnFrontConnected(){
            std::cout<<"front connected"<<std::endl;
            IsConnected_=true;
            login();
        };

        ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
        ///@param nReason 错误原因
        ///        0x1001 网络读失败
        ///        0x1002 网络写失败
        ///        0x2001 接收心跳超时
        ///        0x2002 发送心跳失败
        ///        0x2003 收到错误报文
        virtual void OnFrontDisconnected(int nReason){
            std::cout<<"front disconnected:"<<nReason<<std::endl;  
            IsConnected_=false;          
        };

        ///登录请求响应
        virtual void
        OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){
            std::cout<<__FUNCTION__<<std::endl;
            if (pRspInfo->ErrorID==0){
                IsLoggedIn_=true;
            }
        };

        ///登出请求响应
        virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast){
            if ((pRspInfo!=nullptr)&(pRspInfo->ErrorID==0)){
                IsLoggedIn_=false;
            }
        };

        ///订阅行情应答
        virtual void
        OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast){
            if ((pRspInfo!=nullptr) & (pRspInfo->ErrorID==0)){
                std::cout<<"subscribe success:"<<pSpecificInstrument->InstrumentID<<std::endl;
                if (writers.find( pSpecificInstrument->InstrumentID ) == writers.end()){
                    auto writer=Writer::create("/tmp/trading/testjournal", pSpecificInstrument->InstrumentID);
                    writers[pSpecificInstrument->InstrumentID] = writer;
                    }
            }
        };

        ///取消订阅行情应答
        virtual void
        OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                              bool bIsLast){
        };

        ///深度行情通知
        virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData){
//             std:cout<<pDepthMarketData->InstrumentID<<" "<<pDepthMarketData->LastPrice<<std::endl;
            auto& writer = writers[pDepthMarketData->InstrumentID];
            writer->WriteFrame(static_cast<void *>(pDepthMarketData), mktdata_length_); 
            ++tick_idx;
            TickedTimes[tick_idx] = (writer->frame).getNano();
            strcpy(TickedInstrumentIDs[tick_idx], pDepthMarketData->InstrumentID);
            strcpy(TickExchangeTimes[tick_idx], pDepthMarketData->UpdateTime);
            TickExchangeMillisecs[tick_idx] = pDepthMarketData->UpdateMillisec;
            TickWriteDurations[tick_idx]=getNanoTime() - TickedTimes[tick_idx];
            
        };
        
        void dumptofile(  ){
            int i=0;
            std::ofstream fout;
            fout.open(dumpfilepath_.c_str());
            fout<<"arrivaltime,"<<"exchangetime,"<<"exchangemillisec,"<<"ticker,"<<"latency"<<std::endl;
            for(i=0;i<=tick_idx;i++){
                fout<<TickedTimes[i]<<","<<TickExchangeTimes[i]<<","<<TickExchangeMillisecs[i]<<","<<TickedInstrumentIDs[i]<<","<<TickWriteDurations[i]<<std::endl;
            }
        }


    private:
        int request_id_=0;
        CThostFtdcMdApi *papi_=nullptr;
        bool IsLoggedIn_=false;
        bool IsConnected_=false;
        std::map<std::string, WriterPtr> writers;
        size_t mktdata_length_=sizeof(CThostFtdcDepthMarketDataField);
        size_t tick_idx=-1;
        long long TickWriteDurations[40000*72];
        TThostFtdcInstrumentIDType TickedInstrumentIDs[40000*72];
        long long TickedTimes[40000*72];
        TThostFtdcTimeType TickExchangeTimes[40000*72];
        TThostFtdcMillisecType TickExchangeMillisecs[40000*72];
        const string dumpfilepath_;
};

MarketDataCTP marketdata("/home/feng/mpc-yijinjing-mmap/ctp-run-statistics/ctp_accept_IC2209_stats.csv");
void signal_callback_handler(int signum) {
   std::cout << "Caught signal " << signum << std::endl;
   // Terminate program
   marketdata.dumptofile();
   exit(signum);
};

int main() {
    cpu_set_affinity(3);
    signal(SIGINT, signal_callback_handler);
    marketdata.start();
    while( !marketdata.IsLoggedIn() ){
        usleep(1000000);        
    }
    std::cout<<"Logged in, to collect insts to suscribe"<<std::endl;
    int month=0;
    TThostFtdcInstrumentIDType instrument;
    std::vector<std::string> instruments={"IC2207","IC2208","IC2209","IC2212"};
    marketdata.subscribe(instruments);
    
    std::cout<<"subscribed"<<std::endl;
    marketdata.Join();
//     auto writer=Writer::create("/tmp/trading/testjournal","testj5");
//     int i=-1;
//     CThostFtdcDepthMarketDataField datas[102];
//     size_t length = sizeof(CThostFtdcDepthMarketDataField);
//     std::cout<<"data size="<<length<<std::endl;
//     datas[0].LastPrice=i;
//     writer->WriteFrame(static_cast<void *>(&datas[0]), length); 
//     long long duration = getNanoTime()-(writer->frame).getNano();
//     std::cout<<"time duration:"<<duration <<std::endl;   
//     for(int i=0;i<=100;) {
//         
//         if (duration>125000000){
//            datas[i+1].LastPrice=i;
//     	   writer->WriteFrame(static_cast<void *>(&datas[i+1]), length);
//            duration=getNanoTime()-(writer->frame).getNano();
//            std::cout<<"time duration:"<<duration <<std::endl;
//            ++i;   
// 	}
//         else
//            duration=getNanoTime()-(writer->frame).getNano();
//         
//         
//     }

    return 0;
}
