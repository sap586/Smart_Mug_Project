#include "HX711.h"
#include <SoftwareSerial.h>
#define rxPin 4    // Serial input (connects to Emic 2 SOUT)
#define txPin 2

HX711 scale;
SoftwareSerial mySerial(10, 11); // RX, TX
SoftwareSerial emicSerial =  SoftwareSerial(rxPin, txPin);
int stirPin = 3;
int stirControlPin = 7;
char* IPOfPhone = "AT+CIPSTART=\"TCP\",\"172.20.10.4\",8080";
unsigned long startTime;
unsigned long currentTime;
unsigned long looptime;
float waterConsumption = 0;
float lastWaterConsumption = 0;
int xPin = 8;
int yPin = 12;
float currentWeight;
float lastWeight;
int msgDuration = 15000;
int connection = 0;


void stir() {
  int stirTime = 10;
  analogWrite(stirPin, 1100 - 5);
  delay(1000 * stirTime);
  analogWrite(stirPin, 0);
}

void sendMsgToPhone(String message) {
  
  mySerial.println(IPOfPhone);
  if (mySerial.find("Error")) {
    Serial.println("cant connect to phone");
    return;
  }

  mySerial.print("AT+CIPSEND=");
  mySerial.println(message.length());
  if (mySerial.find(">")) {
    Serial.print(">");
  } else {
    Serial.println("cant send");
    return;
  }

  mySerial.print(message);
  delay(2000);
  while (mySerial.available()) {
    char c = mySerial.read();
    Serial.write(c);
    if (c == '\r')
      Serial.print('\n');
  }
  delay(1000);
}


bool isStable() {
  //acce is verticle about the ground
  long x = pulseIn(xPin, HIGH);
  long y = pulseIn(yPin, HIGH);
  for (int i = 0; i < 300; i++) {
    if (x > 3825 || x < 3745 || y > 5085 || y < 5000) {
      return false;
    }
  }
  return true;
}

void setup() {
  //stirPin
  pinMode(stirPin, OUTPUT);
  pinMode(stirControlPin, INPUT_PULLUP);

  //text-voice
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  emicSerial.begin(9600);
  emicSerial.print('\n');             // Send a CR in case the system is up
  while (emicSerial.read() != ':');   // Wait for ':' character
  delay(10);                          // Short delay
  emicSerial.flush();                 // Flush the receive buffer


  //WIFI
  Serial.begin(9600);
  mySerial.begin(9600);
  mySerial.println("AT+RST");
  mySerial.println("AT+CIPMUX=0");
  //pinMode(LED_BUILTIN, OUTPUT)

   //LoadCell
  Serial.println("Initializing the scale");
  // HX711.DOUT  - pin #A1
  // HX711.PD_SCK - pin #A0
  scale.begin(A1, A0);
  delay(1000);
  scale.set_scale(2280.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();               // reset the scale to 0
  
  delay(1000);
}
void loop() {


  currentTime = millis() - startTime;

  //send Msg every msgDuration
  if (currentTime > msgDuration) {
    String msg = "you have drunk ";
    msg += String(waterConsumption);
    msg += "(OZ) water";
    //Serial.println(msg + "\n");
    sendMsgToPhone(msg + "\n");

    if (waterConsumption - lastWaterConsumption < 4) {
      Serial.println("need water");
      emicSerial.print('S');
      emicSerial.print(msg);
      emicSerial.print(", I think you need drink more, ");
      emicSerial.print("water is very important to your health, ");
      emicSerial.print('\n');
      //while (emicSerial.read() != ':');
    } else {
      emicSerial.print('S');
      emicSerial.print(msg);
      emicSerial.print(", well done ");
      emicSerial.print('\n');
      delay(5);
    }

    // Wait for ":" character
    lastWaterConsumption = waterConsumption;
    startTime = millis();
  }

  //measure Load when the bottle is stable
  if (isStable()) {
    scale.power_up();
    currentWeight = scale.get_units(10);
    scale.power_down();
    if ((lastWeight - currentWeight) > 10) {
      float weightInOz = (lastWeight - currentWeight) / 70 * 4.87;
      Serial.print("Weight: ");
      Serial.println(currentWeight / 68 * 4.87);
      waterConsumption += weightInOz;
      lastWeight = currentWeight;
    }
    else {
      Serial.print("Weight: ");
      Serial.println(currentWeight / 68 * 4.87);
      lastWeight = currentWeight;
    }
  }
  else{
    delay 500;
  }

  //stir
  int stirState = digitalRead(stirControlPin);
  if (stirState == LOW) {
    stir();
  }


}




