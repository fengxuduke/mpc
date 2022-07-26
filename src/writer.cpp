#include <iostream>
#include <fstream>
#include "communicator/Writer.h"
#include "utils/stat.h"
#include <signal.h>
#include "ctpconfig.h"
#include "common.h"
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
                usleep(10000);
            }
            return true;
        }; 


        bool unsubscribe(const std::vector<string> &instruments){
            char* ppInstruments[1];
            for (auto& instrument:instruments){
                ppInstruments[0]=(char*)&instrument[0];
                papi_->UnSubscribeMarketData(ppInstruments, 1);
                usleep(10000);
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
                    auto writer=Writer::create("/tmp/trading/testjournal",pSpecificInstrument->InstrumentID);
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
            writer->WriteFrame(static_cast<void *>(pDepthMarketData), mktdata_length_, MsgType::DepthMarketData); 
            ++tick_idx;
            TickedTimes[tick_idx] = (writer->frame).getNano();
            strcpy(TickedInstrumentIDs[tick_idx], pDepthMarketData->InstrumentID);
            strcpy(TickExchangeTimes[tick_idx], pDepthMarketData->UpdateTime);
            TickExchangeMillisecs[tick_idx] = pDepthMarketData->UpdateMillisec;
            TickWriteDurations[tick_idx]=getNanoTime() - TickedTimes[tick_idx];
            
        };
        
        void dumptofile(  ){
            std::cout<<"begin to dump"<<std::endl;
            int i=0;
            std::ofstream fout;
            fout.open(dumpfilepath_.c_str());
            fout<<"arrivaltime,"<<"exchangetime,"<<"exchangemillisec,"<<"ticker,"<<"latency"<<std::endl;
            for(i=0;i<=tick_idx;i++){
                fout<<TickedTimes[i]<<","<<TickExchangeTimes[i]<<","<<TickExchangeMillisecs[i]<<","<<TickedInstrumentIDs[i]<<","<<TickWriteDurations[i]<<std::endl;
            }
            std::cout<<"finished dumping"<<std::endl;
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

MarketDataCTP marketdata("/home/feng/mpc-yijinjing-mmap/ctp-run-statistics/ctp_accept_stats.csv");
void signal_callback_handler(int signum) {
   std::cout << "Caught signal " << signum << std::endl;
   // Terminate program
   marketdata.dumptofile();
   exit(signum);
};

static const char * const ca_pem_digicert_global_root =
	"-----BEGIN CERTIFICATE-----\nMIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"
	"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
	"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
	"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"
	"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
	"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"
	"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"
	"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"
	"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"
	"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"
	"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"
	"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"
	"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"
	"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"
	"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"
	"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"
	"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"
	"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"
	"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"
	"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"
	"-----END CERTIFICATE-----\n";

int main() {
    cpu_set_affinity(1);
    std::cout<<ca_pem_digicert_global_root<<std::endl;
    signal(SIGINT, signal_callback_handler);
    marketdata.start();
    while( !marketdata.IsLoggedIn() ){
        usleep(1000000);        
    }
    std::cout<<"Logged in, to collect insts to suscribe"<<std::endl;
    std::cout<<"sizeof CThostFtdcDepthMarketDataField="<<sizeof(CThostFtdcDepthMarketDataField)<<std::endl;
    std::cout<<"sizeof char="<<sizeof(char)<<std::endl;
//     std::cout<<"sizeof testram="<<sizeof(testram)<<std::endl;
    int month=0;
    TThostFtdcInstrumentIDType instrument;
    std::vector<std::string> instruments={"IC2208", "IF2208", "IH2208", "IC2209", "IF2209", "IH2209", "IC2212", "IF2212", "IH2212", "IC2303", "IF2303", "IH2303" };
//     std::vector<std::string> instruments;
//     for (auto& item: ExchangeAssets){
//         auto& exchange = item.first;
//         auto& assets = item.second;
//         for (auto& asset:assets){
//             for (month=1;month<=12;month++){
//                 if (exchange == "ZCE"){
//                     continue;
//                     sprintf(instrument, "%s2%02d", asset.c_str(), month);
//                     instruments.push_back(instrument);
//                     
//                     sprintf(instrument, "%s3%02d", asset.c_str(), month);
//                     instruments.push_back(instrument);
//                 }
//                 else if (exchange == "CFFE"){
//                     sprintf(instrument, "%s22%02d", asset.c_str(), month);
//                     instruments.push_back(instrument);
//                     
//                     sprintf(instrument, "%s23%02d", asset.c_str(), month);
//                     instruments.push_back(instrument);
//                     
//                 }
//                 
//             }
//         }
//     }
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
