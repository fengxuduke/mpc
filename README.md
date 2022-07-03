# mmap 进程间通讯

本项目参考和借鉴了[功夫交易系统](https://github.com/taurusai/kungfu)的通讯机制，将功夫中的journal通讯剥离出来，做成了一个IPC的中间件，本项目完全特点是无附加的依赖，直接编译即可运行测试。


*** 此代码并没有经过充分的测试，bug在所难免，后台自动换页操作后续将会实现 ***

# 使用方法

### 启动service 服务

编译后会生成service 可执行程序，此程序对reader和writer进行管理，包括保证每个journal最多只能有一个writer。


### 构建writer

在本项目中，参考功夫，每一个writer只能有一个journal，并且每个journal只可以有一个进程进行写操作，writer构建必须在service启动以后才可以进行，writer构建时会自动尝试链接service并注册writer，如果失败则会抛出异常。以下是writer的构建示例，注意create函数第一个参数是mmap文件存放的文件夹，第二个参数是writer的名称，由于writer只可以有一个journal，所以writer名称和journal名称相同。（此文件在src/writer.cpp中）

```C++

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
```

### 构建reader

本项目中每一个reader可以有多个journal，每次读取时将寻找最近更新的journal的frame。注意由于reader可以拥有多个journal，所以创建reader时回向service注册reader，但是需要手动添加journal。每次readFrame后会自动进行刷新操作。（此文件在src/reader.cpp中）使用reader-readFrame会返回一个指针，当不存在数据时为 nullptr；


```C++

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
```

### 编译样例


```
cd src
mkdir build
cd build
cmake ..
make

```

### 测试运行
mkdir -p /tmp/trading/testjournal /tmp/trading/system

打开一个终端，运行

./service/server

打开第二个终端，运行

./mpcreader

打开第三个终端，运行

./mpcwriter
