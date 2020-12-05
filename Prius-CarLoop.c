//Author: Paul Talaga

// This #include statement was automatically added by the Particle IDE.
// nc -lvu 8888
#include <carloop.h>

#define OBD_CAN_BROADCAST_ID 0x7DF
//SYSTEM_THREAD(ENABLED); // To not block the main program while searching for WiFi


void sendRequestsAtInterval(unsigned long);
unsigned long lastSend;
unsigned long lastSendIndex;
//void printValues();
bool matchRXF(CANMessage message);
//void printMessage(CANMessage message);

void sendRequests();

Carloop<CarloopRevision2> carloop;

int canMessageCount = 0;

char msg[100];
UDP Udp;
IPAddress remoteIP(192, 168, 1, 155);
int port = 8888;

typedef struct msg_type_t{
    short enable;
    char* name;
    unsigned txd04;
    short txd56;
    short rxd;
} msg_type_t;
    
unsigned num_messages = 4;
msg_type_t find_messages[4];


    

void setup() {
    Serial.begin(115200);
    
    Udp.begin(8888);
    
    carloop.begin();
    
    lastSend = millis();
    lastSendIndex = 0;
    
    find_messages[0].enable = 0;
    find_messages[0].name = "Accel Pos";
    find_messages[0].txd04 = 0x024d;   
    find_messages[0].txd56 = 0x0000;
    
    find_messages[1].enable = 1;
    find_messages[1].name = "ICE RPM";
    find_messages[1].txd04 = 0x01CC;   
    find_messages[1].txd56 = 0x0000;
    find_messages[1].rxd = 0x1010; // Was the first 2 bytes changing
    
    find_messages[2].enable = 0;
    find_messages[2].name = "Bat Temp 1 F";
    find_messages[2].txd04 = 0x07E2;   
    find_messages[2].txd56 = 0x2187;
    find_messages[2].rxd = 0x4010;
    
    find_messages[3].enable = 0;
    find_messages[3].name = "Pack Voltage";
    find_messages[3].txd04 = 0x07E2;   
    find_messages[3].txd56 = 0x2174;
    
    
    
    
    
    sprintf(msg,"Here!\n");
    Udp.sendPacket(msg, sizeof(msg), remoteIP, port);
    Serial.print(msg);
    
    /*
    delay(5000); // After Booting, Delay 45 seconds
    
    // Message to transmit
    CANMessage message;
    message.id = OBD_CAN_BROADCAST_ID;
    message.len = 8;
    message.data[0] = 0x02; // Data Length Code (DLC)
    message.data[1] = 0x09; // Mode 09 (https://en.wikipedia.org/wiki/OBD-II_PIDs#Mode_09)
    message.data[2] = 0x02; // PID 02 Vehicle Identification Number (VIN): 17-char ASCII-encoded, left-padded with 0x00
    carloop.can().transmit(message);
    
    strcmp(msg,"Sending VIN message\n");
    Udp.sendPacket(msg, sizeof(msg), remoteIP, port);
    
    Serial.print(msg);
     */
    //delay(1000);
    //Serial.print("SENDING pedal! \n");
    // Message to transmit
    
}

void loop() {
    carloop.update();
    CANMessage message;
    while(carloop.can().receive(message))
    {
        canMessageCount++;
        matchRXF(message);
        sendRequestsAtInterval(200);
    }
    sendRequestsAtInterval(200);
}

void sendRequestsAtInterval(unsigned long interval) {
    unsigned long now = millis();
    if( now - lastSend > interval){
        sendRequests();
        lastSend = millis();
    }
    
}

void printValues(CANMessage message){
    //strcmp(msg,"Sending VIN message");
    Serial.printf("Battery voltage: %12f ", carloop.battery());
    memset(msg, 0, sizeof(msg));
    sprintf(msg, "Battery voltage: %12f \n\0", carloop.battery());
    Udp.sendPacket(msg, sizeof(msg), remoteIP, port);
    
    
    
    Serial.printf("Received %d \n", canMessageCount);
    memset(msg, 0, sizeof(msg));
    sprintf(msg, "Received %d \n", canMessageCount);
    Udp.sendPacket(msg, sizeof(msg), remoteIP, port);
    
    
    return;
    
    // CANMessage contains:
        //      uint32_t id;
        //      bool extended;
        //      bool rtr;
        //      uint8_t len;
        //      uint8_t data[8]
        
}



bool matchRXF(CANMessage message){

    for(int i = 0; i < num_messages; i++){
        //if(message.id == 0x245){
        if(find_messages[i].enable &&  (message.id | 0x07) == ((find_messages[i].txd04 ^ 0x08) | 0x07) ){
            memset(msg, 0, sizeof(msg));
            //sprintf(msg, "match! %04x %04x\n", message.id, find_messages[i].txd04 ^ 0x08);
            sprintf(msg, "match '%s' len: %d ", find_messages[i].name, message.len);
            Udp.sendPacket(msg, sizeof(msg), remoteIP, port);
            for(int j = 0; j < message.len; j++) {
                Serial.printf("%02x ", message.data[j]);
                memset(msg, 0, sizeof(msg));
                sprintf(msg, "%02x ", message.data[j]);
                Udp.sendPacket(msg, sizeof(msg), remoteIP, port);
            }
            memset(msg, 0, sizeof(msg));
            sprintf(msg, "\n    ");
            Udp.sendPacket(msg, sizeof(msg), remoteIP, port);
            
            memset(msg, 0, sizeof(msg));
            sprintf(msg, "shift %d  take %d    \n", find_messages[i].rxd >> 8, find_messages[i].rxd & 0x00ff);
            Udp.sendPacket(msg, sizeof(msg), remoteIP, port);
        }
    }
    //delay(5);
   
}

void sendRequests(){
    //for(int i = 0; i < num_messages; i++){
    int i = lastSendIndex;
        if(find_messages[i].enable && find_messages[i].txd56 != 0x0000){
            CANMessage message;
            message.id = find_messages[i].txd04;
            message.len = 8;
            message.data[0] = 0x02; // Data Length Code (DLC)
            message.data[1] = find_messages[i].txd56 >> 8; // 0x21; // Data Length Code (DLC)
            message.data[2] = find_messages[i].txd56 & 0x00ff; //0x87; // Mode 09 (https://en.wikipedia.org/wiki/OBD-II_PIDs#Mode_09)
            //message.data[2] = 0x74;
            //message.data[2] = 0x02; // PID 02 Vehicle Identification Number (VIN): 17-char ASCII-encoded, left-padded with 0x00
            carloop.can().transmit(message);
            memset(msg, 0, sizeof(msg));
            sprintf(msg, "Sent %s \n", find_messages[i].name);
            Udp.sendPacket(msg, sizeof(msg), remoteIP, port);
            delay(1);
        }
    //}
    lastSendIndex++;
    lastSendIndex = lastSendIndex % num_messages;
}
