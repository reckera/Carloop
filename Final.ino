// This #include statement was automatically added by the Particle IDE.
#include <carloop.h>
#include <string>

Carloop<CarloopRevision2> carloop;

void sendRequestsAtInterval(unsigned long);
void sendRequests();

unsigned long lastSend;
unsigned long lastSendIndex;

typedef struct msg_type_t{
    short enable;
    char* name;
    unsigned pid;
    short txd56;
    short rxd;
} msg_type_t;

unsigned num_messages = 4;
msg_type_t find_messages[4];

int coolant_temp = 0;
String body = "The coolant temperature is 100000000 degrees celsius.";

void setup() {
    Serial.begin(115200);
   
    carloop.begin();
   
    lastSend = millis();
    lastSendIndex = 0;
   
    find_messages[0].enable = 1;
    find_messages[0].name = "ECT";
    find_messages[0].pid = 0x05;
   
    find_messages[1].enable = 1;
    find_messages[1].name = "ICE RPM";
    find_messages[1].pid = 0x0c;
   
    find_messages[2].enable = 1;
    find_messages[2].name = "Vehicle Speed";
    find_messages[2].pid = 0x0d;
   
    find_messages[3].enable = 1;
    find_messages[3].name = "MAF";
    find_messages[3].pid = 0x10;
   
    Serial.printf("Here!\n");
   
    delay(5000); // After Booting, Delay 45 seconds
}

void loop() {
    carloop.update();
    CANMessage message;
    while(carloop.can().receive(message)){
        if(message.data[1] == 0x41){ // If reply to Mode 01
            if(message.data[2] == find_messages[0].pid){ // Match to selected PID(coolant temp)
                Serial.printf("%02x ", message.id);
                for(int j = 0; j < message.len; j++){
                    Serial.printf("%02x ", message.data[j]);
                }
                if(coolant_temp != (message.data[3]-40)){ // If coolant temperature is different send message
                    coolant_temp = message.data[3]-40;
                    body = String("The coolant temperature is currently " + String(coolant_temp) + " degrees celsius.");
                    Particle.publish("twilio_sms", body, PRIVATE);
                }
                Serial.printf("The engine coolant temp is %d degrees celsius\n", coolant_temp);
            }else if(message.data[2] == find_messages[1].pid){ // Match to selected PID(engine RPM)
                Serial.printf("%02x ", message.id);
                for(int j = 0; j < message.len; j++){
                    Serial.printf("%02x ", message.data[j]);
                }
                Serial.printf("The engine RPM is %d\n", (256 * message.data[3] + message.data[4]) / 4);
            }else if(message.data[2] == find_messages[2].pid){ // Match to selected PID(vehicle speed)
                Serial.printf("%02x ", message.id);
                for(int j = 0; j < message.len; j++){
                    Serial.printf("%02x ", message.data[j]);
                }
                Serial.printf("The vehicle speed is %d kilometers per hour\n", message.data[3]);
            }else if(message.data[2] == find_messages[2].pid){ // Match to selected PID(MAF reading)
                Serial.printf("%02x ", message.id);
                for(int j = 0; j < message.len; j++){
                    Serial.printf("%02x ", message.data[j]);
                }
                Serial.printf("The MAF reading is %d grams per second\n", ((256 * message.data[3] + message.data[4]) / 100));
            }
        }
    }
    sendRequestsAtInterval(500);
    delay(1000);
}

void sendRequestsAtInterval(unsigned long interval) {
    unsigned long now = millis();
    if( now - lastSend > interval){
        sendRequests();
        //Serial.printf("sent ");
        lastSend = millis();
    }else{
        //Serial.printf("not sent");
    }
   
}

void sendRequests(){
    int i = lastSendIndex;
    if(find_messages[i].enable){
        CANMessage message;
        message.id = 0x7DF; // diagnostic request ID
        message.len = 8; // message will always be 8 bytes
        //message.extended = true;
        message.data[0] = 0x02; // 2 bytes of data in message
        message.data[1] = 0x01; // Get current data (Mode 01)
        message.data[2] = find_messages[i].pid; // The PID to retrieve (replace with appropriate hex value for the PID)
        //message.data[3] = 0;
        //message.data[4] = 0;
        //message.data[5] = 0;
        //message.data[6] = 0;
        //message.data[7] = 0;
        carloop.can().transmit(message);
    }
    lastSendIndex++;
    lastSendIndex = lastSendIndex % num_messages;
}
