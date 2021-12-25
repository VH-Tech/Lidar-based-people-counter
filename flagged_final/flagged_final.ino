#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <VL53L0X.h>

//Pin connected to ST_CP on the board rck
#define latchPin D5
//Pin connected to SH_CP on the board sck
#define clockPin D6
////Pin connected to DS on the board dio
#define dataPin D8

byte numbers[] = {B00111111,  //0
                  B00110000,  //1
                  B01011011,  //2
                  B01001111,  //3
                  B01100110,  //4
                  B01101101,  //5
                  B01111101,  //6
                  B00000111,  //7
                  B01111111,  //8
                  B01101111}; //9

byte digits[4];
byte powers[5] = { B00001110, B00001101,B00001011,B00000111, B11111111};

#define LDRPin A0 

//#define XSHUT_pin1 not required for address change on Sensor1
#define XSHUT_pin2 D0

//ADDRESS_DEFAULT 0b0101001 or 41
//#define Sensor1_newAddress 41 not required address change
#define Sensor2_newAddress 42


VL53L0X Sensor1;
VL53L0X Sensor2;

bool master_flag = false;
bool slave_1 = false;
bool slave_2 = false;
int prev_time ;

int current_distance1;
int initial_distance1;
int current_distance2;
int initial_distance2;

const char* ssid = "Suresh";  // Enter SSID here
const char* password = "mvls$1488";  //Enter Password here

ESP8266WebServer server(80);

int people = 0;

void setup()
{ //WARNING/
  //Shutdown pins of VL53L0X ACTIVE-LOW-ONLY NO TOLERANT TO 5V will fry them
  pinMode(XSHUT_pin2, OUTPUT);

  
  Serial.begin(9600);
  
  Wire.begin();
  //Change address of sensor and power up next one
   Sensor2.setAddress(Sensor2_newAddress);
  pinMode(XSHUT_pin2, INPUT);
  delay(10);
  
  Sensor1.init();
  Sensor2.init();


  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  Sensor1.startContinuous();
  Sensor2.startContinuous();

  initial_distance1 = Sensor1.readRangeContinuousMillimeters();
  initial_distance2 = Sensor2.readRangeContinuousMillimeters();
  Serial.println("initial_distance1: " + String(initial_distance1));
  Serial.println("initial_distance2: " + String(initial_distance2));

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);

  server.begin();
  Serial.println("HTTP server started");

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  digits[0] = 0;
  digits[1] = 0;
  digits[2] = 0;
  digits[3] = 0;  

  simple1234();
  delay(5000);

}

void loop()
{
  server.handleClient();

  current_distance1 = Sensor1.readRangeContinuousMillimeters();    
  current_distance2 = Sensor2.readRangeContinuousMillimeters();    
  
//  Serial.print(current_distance1);
//  Serial.print(',');
//  Serial.print(current_distance2);
//  Serial.println();

  if(master_flag){
    if(slave_2){
      if(current_distance1 < initial_distance1 - 100){
      master_flag = false;
      slave_2 = false;
      people += 1;
      Serial.println("someone entered");
      checkForLightsOn();
      updateCounter();
      delay(1500);
    }
      }



      if(slave_1){
      if(current_distance2 < initial_distance2 - 100){
      master_flag = false;
      slave_1 = false;
      people -= 1;
      Serial.println("someone exited");
      checkForLightsOff();
      updateCounter();
      delay(1500);
    }
      }

    if(millis() - prev_time > 1000){
      master_flag = false;
      slave_1 = false;
      slave_2 = false;
      Serial.println("flags down");
      }
    
    }

  if(current_distance2 < initial_distance2 - 100 && master_flag == false){
    master_flag = true;
    slave_2 = true;
    Serial.println("raised flag 2");
    prev_time = millis();
    }


    

   if(current_distance1 < initial_distance1 - 100 && master_flag == false){
    master_flag = true;
    slave_1 = true;
    Serial.println("raised flag 1");
    prev_time = millis();
    }

    simple1234();
}

void handleRoot() {
  String webpage;
  webpage += "<html>";
  webpage += "<head><title>Room Strength Tracker</title></head>";
  webpage += "<body>";
  webpage += "<h1>Room Strength Tracker</h1>";
  webpage += "<h1>No. of people in the room : "+String(people)+"</h1>";
  webpage += "</body>";
  webpage += "</html>";
  
  server.send(200, "text/html", webpage);   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void simple1234()
{
  
  for (byte t = 3; t>0; t--)
  { 
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, numbers[digits[t]]);
     ////Serial.println(numbers[digits[t]]);
    shiftOut(dataPin, clockPin, MSBFIRST, powers[t]);
    digitalWrite(latchPin, HIGH);
    delay(5);
  }
}

void checkForLightsOn(){
   if( people == 1 && analogRead(LDRPin) < 400){

                // Use WiFiClient class to create TCP connections
                WiFiClient client;
                HTTPClient http;
                
      String serverPath = "http://192.168.1.150/?socket=2On";
      http.begin(client, serverPath.c_str());
      http.GET(); // Send HTTP GET request    
      Serial.println("Sent HTTP GET request");

        unsigned long timeout = millis();
        while (client.available() == 0) {
          if (millis() - timeout > 5000) {
            Serial.println("Client Timeout !");
            client.stop();
            return;
          }
          yield();   
          }
      
      http.end();
      Serial.println("HTTP end");
    }
  }

void checkForLightsOff(){

if(people == 0){
        // Use WiFiClient class to create TCP connections
          WiFiClient client;
          HTTPClient http;
  
      String serverPath = "http://192.168.1.150/?socket=2Off";
      http.begin(client, serverPath.c_str());
      http.GET(); // Send HTTP GET request   
      Serial.println("Sent HTTP GET request"); 
      
        unsigned long timeout = millis();
        while (client.available() == 0) {
          if (millis() - timeout > 5000) {
            Serial.println("Client Timeout !");
            client.stop();
            return;
          }
          yield();   
          }
        
      http.end();
      Serial.println("HTTP end");
    }
  
  }

void updateCounter(){
  if(people < 0 ){
    people = 0;
    digits[3] = 0;
    digits[2] = 0;
    digits[1] = 0;
    digits[0] = 0;
    }
    
  if(people < 10 ){
    digits[3] = people;
    digits[2] = 0;
    digits[1] = 0;
    digits[0] = 0;
    }
  if(people > 9 && people <100){
    digits[3] = people%10;
    digits[2] = people/10;
    digits[1] = 0;
    digits[0] = 0;
      
     }
}
