#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <CayenneLPP.h>

#define RX_lora 15
#define TX_lora 4
#define RX_gps 23
#define TX_gps 22

EspSoftwareSerial::UART lora_serial;
HardwareSerial gps_serial(2);

TinyGPSPlus gps;
CayenneLPP lpp(51);

double last_lat = 0;
double last_lng = 0;

bool helium_joined = false;

void setup()
{
  Serial.begin(115200);

  lora_serial.begin(9600, SWSERIAL_8N1, RX_lora, TX_lora, false);
  gps_serial.begin(115200, SERIAL_8N1, RX_gps, TX_gps);

  //wait a bit for serail to stabilize
  delay(5000);
  
  //try join Helium
  sendLoraCommand("AT+JOIN=1");
  delay(100);
  checkForLoRaData();
  
  //set GPS at idle
  sendGPSCommand("@GSTP"); //set idle
}

void loop() {
  if (Serial.available()){
    String content = Serial.readString();
    content.trim();
    if(content.startsWith("lora:")) {
      Serial.println("Writing to Lora Module");
      sendLoraCommand(content.substring(5));
    } else if(content.startsWith("gps:")) {
      Serial.println("Writing to GPS Module");
      sendGPSCommand(content.substring(4));
    } else if(content == "current_gps") {
      Serial.println("Current position: " + String(gps.location.lat(), 6) + ", " + String(gps.location.lng(), 6));
      Serial.println("Distance: " + String(gps.distanceBetween(gps.location.lat(), gps.location.lng(), last_lat, last_lng)));
      Serial.println("Altitude: " + String(gps.altitude.meters()));
      Serial.println("Failed Checksum: " + String(gps.failedChecksum()));
      //send out current location
      setCayenneData();
    }
  }

  checkForLoRaData();

  String gps_incomming = "";
  while (gps_serial.available() > 0) {
    char c = (char)gps_serial.read();
    gps_incomming += c;
    if (gps.encode(c)) {
      handleLocation();
    }
  }
  // if(gps_incomming != "") {
  //   Serial.println(gps_incomming);
  // }
}

void checkForLoRaData() {
  if (lora_serial.available()) {
    String incomming = lora_serial.readString();
    incomming.trim();
    Serial.println("Received from Lora Module:");
    Serial.println(incomming);
    if(incomming.indexOf("+EVT:JOINED") >= 0) {
      Serial.println("Joined Helium!");
      helium_joined = true;
      //only init GPS after Helium is joined
      initGPS();
    } else if(incomming.indexOf("+EVT:JOIN FAILED") >= 0) {
      Serial.println("Failed to join Helium :) Retry!");
      sendLoraCommand("AT+JOIN=1");
      helium_joined = false;
    }
  }
}

void sendLoraCommand(String command) {
  command = command + "\r\n";
  char* buf = (char*) malloc(sizeof(char) * command.length() + 1);
  Serial.println(command);
  command.toCharArray(buf, command.length() + 1);
  lora_serial.write(buf);
  free(buf);
}

void sendLoraCayenne(uint8_t *data, uint8_t size) {
  String command = "AT+SEND=1:0:";
  for (unsigned char i = 0; i < size; i++)
  {
    if(data[i] < 0x10) {
      command += '0';
    }
    command += String(data[i], HEX);
  }
  command += "\r\n";
  Serial.println(command);
  //only send data if joined to Helium network
  if(helium_joined) {
    char* buf = (char*) malloc(sizeof(char) * command.length() + 1);
    command.toCharArray(buf, command.length() + 1);
    lora_serial.write(buf);
    free(buf);
  } else {
    Serial.println("Not sent, not joined!");
  }
}

void sendGPSCommand(String command) {
  command = command + "\r\n";
  char* buf = (char*) malloc(sizeof(char) * command.length() + 1);
  Serial.println(command);
  command.toCharArray(buf, command.length() + 1);
  gps_serial.write(buf);
  free(buf);
}

void initGPS() {
  Serial.println("Initializing GPS");

  sendGPSCommand("@GSTP"); //set idle
  delay(100);
  sendGPSCommand("@GNS 815"); //use all satelites
  delay(100);
  sendGPSCommand("@GPPS 1"); //output as soon as clock data is received
  delay(100);
  sendGPSCommand("@GSOP 1 3000 0"); //output data once every 3 seconds
  delay(100);
  sendGPSCommand("@GSR"); //hot start
  delay(100);
}

void handleLocation()
{
  if (gps.location.isValid())
  {
    if(last_lat == 0 || last_lng == 0) {
      //first position, set initial values
      last_lat = gps.location.lat();
      last_lng = gps.location.lng();
      Serial.println("Initial position: " + String(last_lat, 6) + ", " + String(last_lng, 6));
      setCayenneData();
    } else {
      //update last position if distance is greater than 10m
      if(gps.distanceBetween(gps.location.lat(), gps.location.lng(), last_lat, last_lng) > 10) {
        last_lat = gps.location.lat();
        last_lng = gps.location.lng();
        Serial.println("Updated position: " + String(last_lat, 6) + ", " + String(last_lng, 6));
        setCayenneData();
      }
    }
  }
}

void setCayenneData() {
  lpp.reset();
  lpp.addGPS(1, gps.location.lat(), gps.location.lng(), gps.altitude.meters());
  sendLoraCayenne(lpp.getBuffer(), lpp.getSize());
}
