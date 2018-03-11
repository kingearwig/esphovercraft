#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Servo.h>

#define channelcount 3
#define numReadings 25

const char* ssid = "Unstable";
const char* password = "haveyoueverputbutteronapoptart";

WiFiUDP Udp;
unsigned int localUdpPort = 4210;  // local port to listen on
char incomingPacket[255];  // buffer for incoming packets
int chVal[channelcount];
float total[channelcount];
float average[channelcount];
float chreadings[channelcount][numReadings];
int ch;
int readIndex;
Servo servoCh[channelcount];
int chPin[] = {0, 2, 14, 12}; // ESP pins: GPIO 0, 2, 14, 12

void setup()
{
  Serial.begin(115200);
  Serial.println();

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}


void loop()
{
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    // receive incoming UDP packets
    //Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    ch=incomingPacket[2]-'0';
    switch(ch)
    {
      case 0:
        chVal[0]=(int)atof(incomingPacket+4);
        if(chVal[0]<1000)chVal[0]=1000;
        break;
      case 1:
        chVal[1]=(int)atof(incomingPacket+4);
        if(chVal[1]<1000)LiftSet=1000;
        break;
      case 2:
        chVal[2]=(int)atof(incomingPacket+4);
        if(chVal[2]>2000)chVal[2]=2000;
        if(chVal[2]<1000)chVal[2]=1000;
        if(chVal[2]==0)chVal[2]=1500;
        break;
    }
    //Serial.printf("UDP packet contents: %s\n", incomingPacket);
  }

  for(int x=0;x<channelcount;x++)
  {
    total[x]=total[x]-chreadings[x][readIndex];
    chreadings[x][readIndex]=chVal[x];
    total[x]+=chreadings[x][readIndex];
    readIndex++;
    if(readIndex>=numReadings)readIndex=0;
    average[x]=total[x]/numReadings;
  }
  for(int x=0;x<channelcount;x++)
  {
    Serial.print(chVal[x]);Serial.print(" ");
    Serial.print(average[x]); Serial.print(" ");
  }
  Serial.println("");
}
