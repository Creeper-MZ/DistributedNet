#include <Arduino.h>
#include <WiFi.h>
#include <string>
#include <map>
#include <time.h>
using namespace std;
std::map<IPAddress,time_t> TIMELIST;
std::map<IPAddress,time_t>::iterator TIME_ITER;
const char* SSID = "BigDickHandsomeBoy_2.4CM";
const char* PASSWD = "zhang020806#";
WiFiUDP UDP;
time_t *TIME;
const uint16_t PORT = 1000;
short int DEVICELISTLEN=0;
IPAddress str_to_IP(String ip);
void DeviceStatReset();
struct Device {
    Device():IP(IPAddress(0,0,0,0)),Port(0){}
    Device(String ip,uint16_t port):IP(str_to_IP(ip)),Port(port){}
    Device(IPAddress ip,uint16_t port):IP(ip),Port(port){}
    IPAddress IP;
    uint16_t Port;
    float Ability;
    bool operator==(const Device &device){
        return (IP==device.IP&&Port==device.Port);
    }
    bool operator==(const IPAddress &device){
        return (IP==device);
    }
    bool operator!=(const IPAddress &device) {
        return (IP != device);
    }
    bool operator!=(const Device &device) {
        return (IP != device.IP || Port != device.Port);
    }

} DEVICELIST[40];
void sendmsg(const String &msg, const Device device);
void DeviceAdd(const Device &device);
int DeviceCheck(Device Address);
IPAddress GetNetWorkIP();

void ScanLocalIp(){
    for (int i = 1; i < 255; i++) {
        sendmsg("%__CHECK__%", Device(IPAddress(GetNetWorkIP()[0], GetNetWorkIP()[1], GetNetWorkIP()[2], i), PORT));

    }
}
IPAddress GetNetWorkIP(){
    return WiFi.localIP();
}
IPAddress str_to_IP(String ip){
    int pos[5]={0,0,0,0};
    int p=0,i=0;
    while(p!=-1){
        pos[i++]=p;
        p=ip.indexOf('.');
        if(p!=-1)
            ip[p]=' ';
    }
    int add[4]={0,0,0,0};
    pos[0]=-1;
    pos[4]=ip.length();
    for(int i=0,j=1;i<4;i++,j++){
        add[i]=ip.substring(pos[i]+1,pos[j]).toInt();
    }
    return IPAddress(add[0],add[1],add[2],add[3]);
}
IPAddress GetNetworkIP(){
    return WiFi.localIP();
}
void DeviceAdd(const Device &device){
    int pos= DeviceCheck(device);
    if(pos!=-2) {
        DEVICELIST[pos] = device;
        Serial.printf("Device add [%s]",device.IP.toString().c_str());
        Serial.println();
    } else
        return;
    DEVICELISTLEN++;
}
int DeviceCheck(Device Address){
    DeviceStatReset();
    int i=0;
    for(i=0;i<DEVICELISTLEN;i++){
        if(DEVICELIST[i]==Address.IP)
            return -2;
    }
    for(i=0;i<DEVICELISTLEN;i++){
        if(DEVICELIST[i]==IPAddress(0,0,0,0))
            return i;
    }
}
void SendCheckRequest(){
    for(int i=0;i<DEVICELISTLEN;i++){
        if(DEVICELIST[i].Port==0||DEVICELIST[i]==GetNetWorkIP())
            continue;
        sendmsg(String("%__CHECK__%"), DEVICELIST[i]);
    }
}
void Broadcast(const String &str){
    for(int i=0;i<DEVICELISTLEN;i++){
        if(DEVICELIST[i].Port==0||DEVICELIST[i]==GetNetWorkIP())
            continue;
        sendmsg(str, DEVICELIST[i]);
    }
}
void DeviceStatReset(){
    time_t now=time(TIME);
    for(int i=0;i<DEVICELISTLEN;i++){
        if(DEVICELIST[i].Port==0||DEVICELIST[i]==GetNetWorkIP())
            continue;
        long int diff=now-TIMELIST[DEVICELIST[i].IP];
        if(diff>35){
            Serial.printf("Device [%s] Leave",DEVICELIST[i].IP.toString().c_str());
            Serial.println();
            DEVICELIST[i]=Device();
        }
        if(diff>150){
            TIMELIST.erase(DEVICELIST[i].IP);
        }
    }
    while (DEVICELISTLEN>0&&DEVICELIST[DEVICELISTLEN-1].Port==0){
        DEVICELISTLEN--;
    }
}
void UpdateTime(IPAddress IP,time_t tim){
    TIME_ITER = TIMELIST.find(IP);
    if(TIME_ITER==TIMELIST.end()){
        TIMELIST.insert(pair<IPAddress,time_t>(IP,tim));
    }
    else{
        TIME_ITER->second=tim;
    }
}
time_t GetTime(IPAddress IP){
    return TIMELIST[IP];
}
void RequestDecoder(){

}
[[noreturn]] void Server(void *pVoid){
    time_t StartTime=time(TIME);
    Serial.println("Server Thread Begin");
    UDP.begin(PORT);
    while(1) {
        delay(10);
        if((time(TIME)-StartTime)%20==0)
            SendCheckRequest();
        if((time(TIME)-StartTime)%20==0)
            ScanLocalIp();
        int paksize = UDP.parsePacket();
        if (paksize) {
            String str = UDP.readString();
            if(str.startsWith("%__CHECK__%")) {
                // Serial.println("GETCHECK");
                UpdateTime(UDP.remoteIP(),time(TIME));
                sendmsg(String("%__GET__%"), Device(UDP.remoteIP(), PORT));
                DeviceAdd(Device(UDP.remoteIP(), PORT));
            }
            else if(str.startsWith("%__GET__%")){
                // Serial.println("GETRES");
                UpdateTime(UDP.remoteIP(),time(TIME));
                DeviceAdd(Device(UDP.remoteIP(), PORT));
            }
            else
            {
                Serial.printf("Message from[%s:%d]: %s", UDP.remoteIP().toString().c_str(), UDP.remotePort(),str.c_str());
                Serial.println();
            }
        }

    }
    UDP.stop();
}
void sendmsg(const String &msg, const Device device) {
    UDP.beginPacket(device.IP, device.Port);
    UDP.println(msg);
    UDP.endPacket();
}
void ListDevice(){
    Serial.println("---------------------------------------------------------------------------------");
    for(int i=0;i<DEVICELISTLEN;i++) {
        if (DEVICELIST[i].Port == 0)
            continue;
        Serial.printf("ID: %d | IP: %s", i, DEVICELIST[i].IP.toString().c_str()), Serial.println();
    }
    Serial.println("Last Memory: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("---------------------------------------------------------------------------------");
}
[[noreturn]] void setup() {
    for(int i=0;i<40;i++)
        DEVICELIST[i]=Device();
    Serial.begin(115200);
    Serial.println("Connecting");
    String a = Serial.readString();
    Serial.println(a);
    WiFi.begin(SSID, PASSWD);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(". ");
    }
    Serial.println();
    Serial.println("Connected");
    Serial.println(WiFi.localIP());
    //xTaskCreate(Server,"Server",4096/*堆栈大小*/,NULL/*输入参数*/,1/*任务优先级*/,NULL);
    xTaskCreatePinnedToCore(Server,"Server",4096/*堆栈大小*/,NULL/*输入参数*/,1/*任务优先级*/,NULL,0);
    while (1) {
        if (Serial.available()) {
            String msg = Serial.readString();
            ListDevice();
            char c[255];
            memset(c, 0, sizeof(c));
            int i = 0, j = 0;
            for (i = 0; msg[i] != ' '; i++)
                c[i] = msg[i];
            c[i] = '\0';
            String ip(c);
            memset(c, 0, sizeof(c));
            for (j = 0; msg[i] != '\n'; i++, j++)
                c[j] = msg[i];
            c[j] = '\0';
            String por = c;
            int num=por.toInt();
            uint16_t port;
            port = uint16_t(num);
            Serial.print("Send to IP:" + ip + " PORT: ");
            Serial.println(port);
            Serial.println(str_to_IP(ip));
            Device a(ip,port);
            sendmsg("Hello",a);
        }
    }
    // write your initialization code here
}

void loop() {
    // write your code here
}