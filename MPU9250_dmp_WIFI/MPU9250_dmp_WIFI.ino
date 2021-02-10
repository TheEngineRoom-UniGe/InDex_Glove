#include <SparkFunMPU9250-DMP.h>
#include <ros.h>
//#include <MPU9250.h>
#include <std_msgs/String.h>
#include <sensor_msgs/Imu.h>
#include <std_msgs/Header.h>
#include <geometry_msgs/Vector3.h>
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/Pose.h>
#include <WiFi.h>
#include <std_msgs/String.h>
//#include "eeprom_utils.h"
//
//  float getQuaternion(uint8_t i) const { return (i < 4) ? q[i] : 0.f; }
//
//    float getAcc(uint8_t i) const { return (i < 3) ? a[i] : 0.f; }
//    float getGyro(uint8_t i) const { return (i < 3) ? g[i] : 0.f; }
//    float getMag(uint8_t i) const { return (i < 3) ? m[i] : 0.f; }
// to get smaller array these are the raw functions
//
//
// ADC on 4D address 
MPU9250_DMP mpu; 

const char* ssid = "EmaroLab-WiFi";
const char* password = "walkingicub";
IPAddress server (130, 251, 13, 113); //(192,168,43,94);//// ip of your ROS server
IPAddress ip;  
int status = WL_IDLE_STATUS;
char a = 51;
int8_t P[12] = {0,1,2,3,4,5,15,16,17,18,19,20};
WiFiClient client;
unsigned int localPort = 2390;
WiFiClient clientDebug;
unsigned int debugPort = 2490;
WiFiUDP udpDebug; 

typedef union {
 float floatingPoint[4];
 byte binary[16];
} binary4Float;

typedef union{
  int8_t integerPoint;
  byte binary;
} binaryInt;

typedef union{
  int16_t integerPoint[3];
  byte binary[6];
} binary3Int;

void sendData(  geometry_msgs::Vector3 accel, geometry_msgs::Vector3 gyro,  geometry_msgs::Quaternion q, int8_t id){
  binaryInt ID;
  ID.integerPoint = id;
  WiFiUDP Udp;
  binary4Float quaternion;
  quaternion.floatingPoint[0] = q.x;
  quaternion.floatingPoint[1] = q.y;
  quaternion.floatingPoint[2] = q.z;
  quaternion.floatingPoint[3] = q.w;

  binary3Int accelerometer;
  accelerometer.integerPoint[0] = accel.x;
  accelerometer.integerPoint[1] = accel.y;
  accelerometer.integerPoint[2] = accel.z;

  binary3Int gyroscope;
  gyroscope.integerPoint[0] = gyro.x;
  gyroscope.integerPoint[1] = gyro.y;
  gyroscope.integerPoint[2] = gyro.z;

  byte packetBuffer[29];
  memset(packetBuffer, 0, 29);

  packetBuffer[28] = ID.binary;
 // packetBuffer[16] = ID.binary;
  
  for(int i=0; i<16; i++){
    packetBuffer[i] = quaternion.binary[i];
  }
  
  for(int i=16; i<22; i++){
    packetBuffer[i] = accelerometer.binary[i-16];
  }

  for(int i=22; i<28; i++){
    packetBuffer[i] = gyroscope.binary[i-22];
  }

  Udp.beginPacket(server, localPort);
  Udp.write(packetBuffer, 29);
  Udp.endPacket();  
}



class WiFiHardware {

  public:
  WiFiHardware() {};

  void init() {
    // do your initialization here. this probably includes TCP server/client setup
    client.connect(server, 11411);

    
    
  }

  // read a byte from the serial port. -1 = failure
  int read() {
    // implement this method so that it reads a byte from the TsendCP connection and returns it
    //  you may return -1 is there is an error; for example if the TCP connection is not open
    return client.read();         //will return -1 when it will works
  }

  // write data to the connection to ROS
  void write(uint8_t* data, int length) {
    // implement this so that it takes the arguments and writes or prints them to the TCP connection
    for(int i=0; i<length; i++)
      client.write(data[i]);
  }

  // returns milliseconds since start of program
  unsigned long time() {
     return millis(); // easy; did this one for you
  }
};

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(0x70);
  Wire.write(1 << i);
  Wire.endTransmission();  
}
 
void i2cTest() {
  byte error, address;
  int nDevices;
  delay(500);
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("found ");Serial.print(nDevices);
  }
  delay(1000);          
}
void setupWiFi()
{ bool onoff=true;
  WiFi.begin(ssid, password);
  //Print to serial to find out IP address and debugging
  Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) 
  {
    delay(500);
    onoff=!onoff;
    digitalWrite (LED_BUILTIN, onoff);
    
//    digitalWrite (D9, onoff);
    Serial.print(".....");

  }
  if(i == 21){
    Serial.print("Could not connect to"); Serial.println(ssid);
    while(1) delay(500);
  }
  Serial.print("Ready! Use ");
  ip = WiFi.localIP();
  Serial.print(ip);
  Serial.println(" to access client");
  

}
geometry_msgs::Vector3 acc;
geometry_msgs::Vector3 gyr;
geometry_msgs::Quaternion q;
void setup()
{  
    pinMode(D2,INPUT);
    pinMode(D3,OUTPUT); 
    pinMode(D9,OUTPUT);  // LED PIN
    digitalWrite(D3,HIGH);
  //  EEPROM.begin(512); // otherwise it wont write
    Serial.begin(115200);  
    setupWiFi();
    clientDebug.connect(server,11511); // new debug port
    
    Serial.println("Here I Am");
    for (int i =0;i <5;i ++)
    {
    digitalWrite(LED_BUILTIN, HIGH); 
    delay(500);  
    digitalWrite(LED_BUILTIN, LOW); 
    delay(200);  
   // digitalWrite(LED_BUILTIN, HIGH); 
    digitalWrite(D9, HIGH); 
    // does not work when on usb
    }
    acc.x = 0;  acc.y = 0;  acc.z =0; gyr.x = 0;  gyr.y =0;  gyr.z = 0;



    //Serial.println("I am Ready for I2C");
    //DEFAULT ADDRESS 0X68
    if (mpu.begin() != INV_SUCCESS)
  {
    while (1)
    {
      Serial.println("Unable to communicate with MPU-9250");
      Serial.println("Check connections, and try again.");
      Serial.println();
      delay(5000);
    }
  }
  
  mpu.dmpBegin(DMP_FEATURE_6X_LP_QUAT | // Enable 6-axis quat
               DMP_FEATURE_GYRO_CAL, // Use gyro calibration
              200); // Set DMP FIFO rate to 200 Hz
  // DMP_FEATURE_LP_QUAT can also be used. It uses the 
  // accelerometer in low-power mode to estimate quat's.
  // DMP_FEATURE_LP_QUAT and 6X_LP_QUAT are mutually exclusive

}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH); 
    static uint32_t prev_ms = millis();
    if ((millis() - prev_ms) > 20)
      {
        tcaselect(6);
       // delay(5); // with 10, 20 ms , works fine , always updating ,visualised using teapot // 10 ms seems more stable (26fps with teapot)
          if ( mpu.fifoAvailable() )
  {
    // Use dmpUpdateFifo to update the ax, gx, mx, etc. values
    if ( mpu.dmpUpdateFifo() == INV_SUCCESS)
    {
      // computeEulerAngles can be used -- after updating the
      // quaternion values -- to estimate roll, pitch, and yaw
  //    mpu.computeEulerAngles();
      
      float q0 = mpu.calcQuat(mpu.qw);
      float q1 = mpu.calcQuat(mpu.qx);
      float q2 = mpu.calcQuat(mpu.qy);
      float q3 = mpu.calcQuat(mpu.qz);
      Serial.print("w");Serial.print(q0);Serial.print("w");
      Serial.print("a");Serial.print(q1);Serial.print("a");
      Serial.print("b");Serial.print(q2);Serial.print("b");
      Serial.print("c");Serial.print(q3);Serial.println("c");
      
   //q.x = mpu.getQuaternion(0);  q.y = mpu.getQuaternion(1);  q.z = mpu.getQuaternion(2);  q.w = mpu.getQuaternion(3);
//    acc.x = mpu.getAcc(0); acc.y = mpu.getAcc(1); acc.x = mpu.getAcc(2);
//    gyr.x = mpu.getGyro(0); gyr.y = mpu.getGyro(1); gyr.z = mpu.getGyro(2);
//    
    q.x = q1 ; q.y = q2 ; q.z = q3 ; q.w = q0; 
   // acc.x = mpu.getAcc(0); acc.y = mpu.getAcc(1); acc.x = mpu.getAcc(2);
    //gyr.x = mpu.getGyro(0); gyr.y = mpu.getGyro(1); gyr.z = mpu.getGyro(2);
   sendData( acc,  gyr,   q, P[1]);


      
     // printIMUData();
    }
  }
}
        
        
 //       mpu.update();
       // mpu.printRawData();
//      Serial.print("pitch");Serial.println(mpu.getPitch()); 
//      Serial.print("roll");Serial.println(mpu.getRoll()); 
      
      //Serial.print(mpu.getTemperature());
      // the below code works with the PYteapot .
      // to read the temperature , we have set the AHRS to false
     // Serial.print("t1");Serial.print(mpu.getTemperature());Serial.print("t");
    
    }
//}
