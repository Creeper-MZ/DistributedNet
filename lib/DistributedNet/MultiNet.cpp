//
// Created by e1550 on 2022/4/30.
//
#include "MultiNet.h"
MIPAddress::MIPAddress(const String &IP) {
    MIPAddress(fromString(IP));
}
bool MIPAddress::operator<(const MIPAddress &a) {
    return (*this)[3]<a[3];
}
bool MIPAddress::operator>(const MIPAddress &a) {
    return (*this)[3]>a[3];
}
MultiNet::~MultiNet() {
    Serial.println("Done");//测试用，出现了问题，需要再MultiNet内部集成Wifi连接的功能，并且要单独列一个begin开启MultiNet服务
}
void MultiNet::Listen(void *pVoid) {//由于此函数为静态函数，所以无法直接引用成员变量。做了一些处理，用pair指针打包MultiNet对象和端口，这样就可以通过MultiNet对象的Listener成员变量通过端口找到服务器。
    std::pair<MultiNet*,uint16_t>* tmp;
    tmp=(std::pair<MultiNet*,uint16_t>*)pVoid;//提取到pair指针
    time_t ScanTime=time(tmp->first->TIME);
    delay(1);//等待Lintener数据插入成功
    uint16_t Port=tmp->second;//获得端口first为MultiNet对象
    WiFiUDP *UDP=tmp->first->Listener[Port].first;
    //Serial.printf("UDP:%p,MultNet:%p,Port:%d",(int)UDP,(int)(tmp->first),tmp->second);
    Device *device;
    while(1) {
        if(tmp->first->AUTOSCANTIME!=0&&time(tmp->first->TIME)-ScanTime>=tmp->first->AUTOSCANTIME){
            tmp->first->CheckOnlineStat();//先优先对已存在设备进行存货状态检查
            tmp->first->ScanNow();//对网段进行扫描
            ScanTime=time(tmp->first->TIME);
        }//自动扫描设备，附加到Listen进程
        delay(5);
        if (UDP->parsePacket()) {
            String MSG=UDP->readString();//获得传来数据
            device=new Device(MIPAddress(UDP->remoteIP()),Port);
            (tmp->first)->MsgBuff.push(std::pair<Device*,String>(device,MSG));//将传来的数据插入到待处理队列
            if(tmp->first->DEVICELIST.find(device->IP)==tmp->first->DEVICELIST.end()){
                tmp->first->DEVICELIST.insert(std::pair<MIPAddress,Device*>(device->IP,device));//将发送消息的设备添加到设备列表
                tmp->first->TIMELIST.insert(std::pair<Device*,time_t>(device, time(tmp->first->TIME)));
            }
            else{
                tmp->first->TIMELIST[tmp->first->DEVICELIST[device->IP]]=time(tmp->first->TIME);//进行时间表维护
                delete device;
            }
        }
    }
}
void MultiNet::Scanner(void *pVoid){
    ((MultiNet*)pVoid)->ISSCANNING=1;//将扫描状态设置为在扫描
    MIPAddress Local=MIPAddress(WiFi.localIP());//获得本地网段。
    for(int i=1;i<=254;i++){
        delay(5);
        MIPAddress IP(Local[0],Local[1],Local[2],i);
        ((MultiNet*)pVoid)->SendMsg(IP,String("<|C|>"));//通过向所有网段发送<|C|>请求来获得他们的名字和存活状态
    }
    ((MultiNet*)pVoid)->ISSCANNING=0;//扫描完毕
}

[[noreturn]] void MultiNet::Decoder(void *pVoid) {//这是一个指令解析器，将所有存放在Msg表里面的指令或者消息进行解析
    MultiNet* tmp=(MultiNet*)pVoid;//获得MultiNet对象
    while (1){
        delay(5);//在多线程状态下通过对此进程进行延迟来给其他进程程序时间
        if(tmp->MsgBuff.empty())//如果没有指令
            continue;
        std::pair<Device*,String> &t=tmp->MsgBuff.front();//MsgBuff以队列形式储存，这个为获得队首，并用用t引用
        if(t.first->IP==WiFi.localIP()){
            tmp->MsgBuff.pop();
            continue;
        }
        if(t.second.startsWith("<|")){//判断是否为指令，如果不是，则为普通消息
            switch (t.second[2]) {//解析具体指令
                case 'C':if(tmp->CANBEFOUND)tmp->SendMsg(*t.first,String("<|N|")+tmp->NAME);break;//此指令为检查指令，返回此对象的名字"<|N|NAME"
                case 'N':t.first->Name=t.second.substring(4,t.second.length()-1);break;//解析发来的名字
                case ':':tmp->OtherRequestDecoder(t.second.substring(4,t.second.length()));break;//支持自定义指令，通过重载OtherRequestDecoder方法格式为<|:|Request
            }
        }
        else
        {
            Serial.printf("Message From[ %s : %d ] -> %s",t.first->IP.toString().c_str(),t.first->Port,t.second.c_str());//打印消息
            Serial.println();
        }

        tmp->MsgBuff.pop();
    }
}
void MultiNet::CheckOnlineStat(){//检查当前列表设备是否在线
    TIME_ITER=TIMELIST.begin();
    for(TIME_ITER;TIME_ITER!=TIMELIST.end();TIME_ITER++)
        SendMsg(*TIME_ITER->first,String("<|C|>"));
    TIME_ITER=TIMELIST.begin();
    for(TIME_ITER;TIME_ITER!=TIMELIST.end();TIME_ITER++){
        if(time(TIME)-TIME_ITER->second>=DEVICETIMEOUT) {
            Serial.printf("Device IP: %s Leave",TIME_ITER->first->IP.toString().c_str());
            DEVICELIST.erase(TIME_ITER->first->IP);
        }
        if(time(TIME)-TIME_ITER->second>=DEVICETIMEOUT+100) {
            TIMELIST.erase(TIME_ITER->first);
        }
    }
}
MultiNet::MultiNet(const uint16_t &ListenAndSendPort,const String &name) {
    SENDPORT=ListenAndSendPort;
    LISTENBEGIN=ListenAndSendPort;
    LISTENEND=ListenAndSendPort;
    AUTOSCANTIME=20;
    DEVICENUM=0;
    CANBEFOUND=1;
    DEVICETIMEOUT=45;
    ISSCANNING=0;
    NAME=name;
}
MultiNet::MultiNet(const uint16_t &ListenPort, const uint16_t &SendPort, const String &name) {
    SENDPORT=SendPort;
    LISTENBEGIN=ListenPort;
    LISTENEND=ListenPort;
    AUTOSCANTIME=20;
    DEVICENUM=0;
    CANBEFOUND=1;
    DEVICETIMEOUT=45;
    ISSCANNING=0;
    NAME=name;
}
MultiNet::MultiNet(const uint16_t &ListenPortStartAt, const uint16_t &ListenPortEndAt, const uint16_t &SendPort,const String &name) {
    SENDPORT=SendPort;
    LISTENBEGIN=ListenPortStartAt;
    LISTENEND=ListenPortEndAt;
    AUTOSCANTIME=20;
    DEVICENUM=0;
    CANBEFOUND=1;
    DEVICETIMEOUT=45;
    ISSCANNING=0;
    NAME=name;
}
WiFiUDP *MultiNet::NewListen(const uint16_t &Port) {
    std::pair<MultiNet*,uint16_t>* PACKEG=new std::pair<MultiNet*,uint16_t>(this,Port);
    WiFiUDP* UDP= new WiFiUDP;//创造一个服务器指针，之后会和服务器进程一起和端口封装到一起
    //Serial.printf("UDP:d,MultNet:%p",PACKEG);
    TaskHandle_t xThreat=NULL;
    UDP->begin(Port);
    xTaskCreatePinnedToCore(Listen,String(Port).c_str(),4096/*堆栈大小*/,PACKEG/*输入参数*/,1/*任务优先级*/,&xThreat,0);
    Listener.insert(std::pair<uint16_t,std::pair<WiFiUDP*,TaskHandle_t>>(Port,std::pair<WiFiUDP*,TaskHandle_t>(UDP,xThreat)));
    return UDP;
}
bool MultiNet::StopListen(const uint16_t &Port) {
    delay(5);
    try {
        Listener[Port].first->stop();
    }
    catch (...) {
        return 0;
    }
    vTaskDelete(Listener[Port].second);
    delete Listener[Port].first;
    Listener.erase(Port);
    //Serial.println("Listener stoped");
    return 1;
}

void MultiNet::GetDeviceList() {
    std::map<MIPAddress,Device*>::iterator D_ITER2=DEVICELIST.begin();
    for(D_ITER2;D_ITER2!=DEVICELIST.end();D_ITER2++)
        Serial.printf("Device[%s] IP:%s Port:%d",D_ITER2->second->Name.c_str(),D_ITER2->second->IP.toString().c_str(),D_ITER2->second->Port),Serial.println();
}
Device *MultiNet::FindDevice(const MIPAddress &IP) {
    return DEVICELIST[IP];
}
void MultiNet::SetDeviceTimeout(const uint32_t &timeout) {
    DEVICETIMEOUT=timeout;
}

bool MultiNet::OtherRequestDecoder(const String &request) {
    if(request.equals("showdevice"))
        GetDeviceList();
}
void MultiNet::TimeOfAutoScan(const uint32_t &minsecond) {
    DEVICETIMEOUT=minsecond;
}

uint32_t MultiNet::GetTimeOfAutoScan() {
    return DEVICETIMEOUT;
}

uint16_t MultiNet::GetSendPort() {
    return SENDPORT;
}
bool MultiNet::IfCanBeFound() {
    return CANBEFOUND;
}
void MultiNet::ScanNow() {
    Scanner(this);
    //MultiNet* Net=this;
    //xTaskCreatePinnedToCore(Scanner,WiFi.localIP().toString().c_str(),4096/*堆栈大小*/,Net/*输入参数*/,1/*任务优先级*/, NULL,1);//多线程扫描
}
void MultiNet::CanBeFound() {
    CANBEFOUND=1;
}
void MultiNet::CannotBeFound() {
    CANBEFOUND=0;
}
void MultiNet::SendMsg(const MIPAddress &IP, const String &Msg) {
    SendServer.beginPacket(IP,SENDPORT);
    SendServer.print(Msg.c_str());
    SendServer.endPacket();
}
void MultiNet::SendMsg(const Device &IP, const String &Msg) {
    SendServer.beginPacket(IP.IP,SENDPORT);
    SendServer.print(Msg.c_str());
    SendServer.endPacket();
}
void MultiNet::Begin(){
    if(LISTENBEGIN==LISTENEND)
    NewListen(LISTENBEGIN);
    else{
        for(int i=LISTENBEGIN;i<=LISTENEND;i++)
            NewListen(i);
    }
    Device *My=new Device(MIPAddress(WiFi.localIP()),SENDPORT);
    My->Name=NAME;
    DEVICELIST.insert(std::pair<MIPAddress,Device*>(MIPAddress(WiFi.localIP()),My));
    xTaskCreatePinnedToCore(Decoder,NAME.c_str(),4096/*堆栈大小*/,this/*输入参数*/,1/*任务优先级*/,NULL,1);
}
int MultiNet::NumOfLintener() {
    return Listener.size();
}
bool MultiNet::IsScanning() {
    return ISSCANNING;
}