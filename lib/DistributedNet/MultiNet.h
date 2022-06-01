//
// Created by e1550 on 2022/4/30.
//
#ifndef MULTNET_MULTNET_H
#define MULTNET_MULTNET_H
#include <WiFi.h>
#include <map>
#include <queue>
class MIPAddress:public IPAddress{
public:
    MIPAddress(const String &IP);
    using IPAddress::fromString;
    using IPAddress::toString;
    using IPAddress::IPAddress;
    using IPAddress::operator[];
    using IPAddress::operator=;
    using IPAddress::operator==;
    using IPAddress::operator unsigned int;
    bool operator>(const MIPAddress &a);
    bool operator<(const MIPAddress &a);
};
struct Device {

    MIPAddress IP;
    uint16_t Port;
    uint16_t UDPPort;
    bool IfTCP,Begin,End; //用这个标计队首和队尾
    WiFiServer *TCP;
    String Name;
    Device():IP(MIPAddress(0,0,0,0)),Port(0){}
    Device(String ip,uint16_t port):IP(ip),Port(port){}
    Device(MIPAddress ip,uint16_t port):IP(ip),Port(port){}
    bool operator==(const Device &device){
        return (IP==device.IP&&Port==device.Port);
    }
    bool operator==(const MIPAddress &device) {
        return (IP == device);
    }
    bool operator!=(const MIPAddress &device) {
        return (IP != device);
    }
    bool operator!=(const Device &device) {
        return (IP != device.IP || Port != device.Port);
    }
    bool operator>(const Device &device){
        return (IP>device.IP);
    }
    bool operator<(const Device &device){
        return (IP<device.IP);
    }

};

class MultiNet {//分配空间时一定要注意，PSRAM的开启状态。使用动态内存分配来达到使用片外PSRAM的目的
private:
    bool ISSCANNING;
    String NAME;
    Device* DEVICEBEGIN;
    std::map<MIPAddress,Device*> DEVICELIST;//维护一个设备列表
    std::map<Device*,time_t> TIMELIST;//维护一个最后收到设备消息的时间表
    std::map<Device*,time_t>::iterator TIME_ITER;//时间表迭代器
    std::map<uint16_t,std::pair<WiFiUDP*,TaskHandle_t>> Listener;//方便通过端口关闭监听服务器
    std::queue<std::pair<Device*,String>> MsgBuff;//待处理消息队列
    //std::map<Device*,String> MsgBuff;
    WiFiUDP SendServer;//发送消息用的UDP客户端
    WiFiServer *TCP;//未来准备要使用的TCP服务器，发送文件之前必须要建立的服务
    time_t *TIME;
    uint32_t DEVICETIMEOUT;//没有收到设备消息超过这个时间就自动移除设备
    uint16_t SENDPORT;//发送和扫描端口（默认）
    uint16_t LISTENBEGIN,LISTENEND;
    uint8_t DEVICENUM;
    uint32_t AUTOSCANTIME;//自动扫描时间间隔，设置为0关闭自动扫描
    bool ENABLEPSRAM,CANBEFOUND;//未来要利用上PSRAM。开启和关闭设备发现
public:
    static void Listen(void *pVoid);//多线程的监听器
    static void Scanner(void *pVoid);//多线程的扫描器
    [[noreturn]] static void Decoder(void *pVoid);
    MultiNet(const uint16_t &ListenAndSendPort,const String &name);//默创建Linsen 每个UDP ，TCP，DEVICE的大小都设置为
    MultiNet(const uint16_t &ListenPort,const uint16_t &SendPort,const String &name);//如果ListenPort为0，就不创建ListenServer
    MultiNet(const uint16_t &ListenPortStartAt,const uint16_t &ListenPortEndAt,const uint16_t &SendPort,const String &name);//未来实现TCP通道的时候注意不能和这些端口重叠
    ~MultiNet();
    void AllOpenTCP();//将所有设备变成TCP连接，端口自动设置，在维护的设备列表中去更新端口
    void AllCloseTCP();//将所有设备取消TCP连接，端口恢复为之前的UDP端口。
    WiFiServer *TCPStart(Device &device);//使用随机TCP端口 先向远端设备请求一个端口，之后
    WiFiServer *TCPStart(Device &device,const uint16_t &Port);//使用指定TCP端口,这两个TCP会记录到Device里的TCP
    void CheckOnlineStat();//用来更新设备每次扫描完毕之后，通过TIMELIST
    WiFiUDP* NewListen(const uint16_t &Port);
    bool StopListen(const uint16_t &Port);
    void GetDeviceList();
    Device *FindDevice(const MIPAddress &IP);
    uint16_t GetAvlibleTCPPort();//使用异常捕获机制加对端口表的检查来检查端口是否可用
    void SetDeviceTimeout(const uint32_t &timeout);//设置设备没有相应之后删除的时间
    bool OtherRequestDecoder(const String &request);
    bool IfPortAvlible(const uint16_t &Port);
    void TimeOfAutoScan(const uint32_t &minsecond);//设置自动扫描时间，如果为0关闭自动扫描
    uint32_t GetTimeOfAutoScan();
    uint16_t GetSendPort();
    bool IfEnablePsram();//设置是否开启PSRAM
    bool IfCanBeFound();//获取设备是否可被找到
    void ScanNow();//手动扫描默认通过SendPort扫描
    void ScanNow(const uint16_t &PortAt);
    void ScanNow(const uint16_t &PortStartAt,const uint16_t &PortEndAt);//端口区间扫描
    void CanBeFound();
    void CannotBeFound();//关闭/开启设备发现
    void SendMsg(const MIPAddress &IP,const String &Msg);
    //void SendMsg(const String &IP,const String &Msg);
    void SendMsg(const Device &device,const String &Msg);
    void SendFile(const MIPAddress &IP,const String &File);//必须要建立tcp才可以
    //void SendFile(const String &IP,const String &File);
    void SendFile(const Device &device,const String &File);
    void GetDeviceListFrom(const Device *remotedevice);
    void LetRemoteDevice(const Device );//让远程设备执行任务1
    void Begin();
    int NumOfLintener();
    bool IsScanning();
};


#endif MULTNET_MULTNET_H
