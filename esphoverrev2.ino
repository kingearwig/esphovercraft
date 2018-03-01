// 4-channel RC receiver for controlling
// an RC car / boat / plane / quadcopter / etc.
// using an ESP8266 and an Android phone with RoboRemo app

// Disclaimer: Don't use RoboRemo for life support systems
// or any other situations where system failure may affect
// user or environmental safety.

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Servo.h>
#include <Wire.h>
#define numReadings 100
const int MPU_addr=0x68; //I2C address of the MPU-6050
int16_t GyZ;//,total,yawReadings[numReadings],GyZTOT,correction,average;
float total,yawReadings[numReadings],GyZTOT,correction,average;
int readIndex;
bool overRide=0;
unsigned long lastTime=micros();
// config:

const char *ssid = "ESPHoverCraft";  // You will connect your phone to this Access Point
const char *pw = "qwerty123"; // and this is the password
IPAddress ip(192, 168, 0, 1); // From RoboRemo app, connect to this IP
IPAddress netmask(255, 255, 255, 0);
const int port = 9876; // and this port

const int chCount = 4; // 4 channels, you can add more if you have GPIOs :)
Servo servoCh[chCount]; // will generate 4 servo PWM signals
int chPin[] = {0, 2, 14, 12}; // ESP pins: GPIO 0, 2, 14, 12
int chVal[] = {1000, 1000, 1500, 1500}; // default value (middle)

int usMin = 700; // min pulse width
int usMax = 2300; // max pulse width


WiFiServer server(port);
WiFiClient client;


char cmd[100]; // stores the command chars received from RoboRemo
int cmdIndex;
unsigned long lastCmdTime = 60000;
unsigned long aliveSentTime = 0;


boolean cmdStartsWith(const char *st)
{ // checks if cmd starts with st
  for(int i=0; ; i++)
  {
    if(st[i]==0) return true;
    if(cmd[i]==0) return false;
    if(cmd[i]!=st[i]) return false;;
  }
  return false;
}


void exeCmd() { // executes the command from cmd

  lastCmdTime = millis();

  // example: set RoboRemo slider id to "ch0", set min 1000 and set max 2000
  
  if( cmdStartsWith("ch") )
  {
    int ch = cmd[2] - '0';
    if(ch>=0 && ch<=9 && cmd[3]==' ') {
      chVal[ch] = (int)atof(cmd+4);
    }
  } 
}



void setup() {
  chVal[0]=0;
  chVal[1]=1000;
  chVal[2]=1500;
  for(int x=0;x<=chCount;x++)
  {
    servoCh[x].attach(chPin[x], usMin, usMax);
    servoCh[x].writeMicroseconds(chVal[x]);
  }
  
  delay(1000);

  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);  
  
  cmdIndex = 0;

  
  
  Serial.begin(115200);

  WiFi.softAPConfig(ip, ip, netmask); // configure ip address for softAP 
  WiFi.softAP(ssid, pw); // configure ssid and password for softAP

  server.begin(); // start TCP server

  Serial.println("ESP8266 RC receiver 1.1 powered by RoboRemo");
  Serial.println((String)"SSID: " + ssid + "  PASS: " + pw);
  Serial.println((String)"RoboRemo app must connect to " + ip.toString() + ":" + port);
  
}


void loop() {
  //chVal[2]=1500;
  // if contact lost for more than half second
  /*if(millis() - lastCmdTime > 500) {  
    for(int i=0; i<chCount; i++) {
      // set all values to middle
      Serial.print(chVal[i]);Serial.print(" | ");
      servoCh[i].writeMicroseconds( (usMin + usMax)/2 );
      servoCh[i].detach(); // stop PWM signals
    }
    Serial.println("");
  }*/

  Wire.beginTransmission(MPU_addr);
  Wire.write(0x47);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,2,true);  // request a total of 14 registers
  
  //AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
  //AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  //AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  //Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  //GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  //GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
  
  Serial.print(GyZ);
  
    total=total-yawReadings[readIndex];
    yawReadings[readIndex]=GyZ;
    total+=yawReadings[readIndex];
    readIndex++;
    if(readIndex>=numReadings)readIndex=0;
    average=total/numReadings;
  Serial.print(" | ");Serial.print(average);
  correction=map(average,-300,300,1000,2000);
  correction=constrain(correction,1000,2000);
  Serial.print(" | ");Serial.println(chVal[2]);

  
  if(!client.connected()) {
    client = server.available();
    return;
  }

  // here we have a connected client

  if(client.available()) {
    char c = (char)client.read(); // read char from client (RoboRemo app)

    if(c=='\n') { // if it is command ending
      cmd[cmdIndex] = 0;
      exeCmd();  // execute the command
      cmdIndex = 0; // reset the cmdIndex
    } else {      
      cmd[cmdIndex] = c; // add to the cmd buffer
      if(cmdIndex<99) cmdIndex++;
    }
  } 

  if(millis() - aliveSentTime > 500) { // every 500ms
    client.write("alive 1\n");
    // send the alibe signal, so the "connected" LED in RoboRemo will stay ON
    // (the LED must have the id set to "alive")
    
    aliveSentTime = millis();
    // if the connection is lost, the RoboRemo will not receive the alive signal anymore,
    // and the LED will turn off (because it has the "on timeout" set to 700 (ms) )
  }

  
  //if(chVal[2]=1500)
  //{
  //  chVal[2]=correction;
  //}
  for(int x=0;x<=chCount;x++)
  {
    servoCh[x].writeMicroseconds(chVal[x]);
  }
  if(abs(chVal[2]-1500)>=10)servoCh[2].writeMicroseconds(chVal[2]);
  if(abs(chVal[2]-1500)<10)servoCh[2].writeMicroseconds(correction);


}
