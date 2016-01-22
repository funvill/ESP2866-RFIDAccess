/*
Many thanks to nikxha from the ESP8266 forum
https://github.com/Jorgen-VikingGod/ESP8266-MFRC522 

required libary: https://github.com/miguelbalboa/rfid
*/

#include <ESP8266WiFi.h>
#include <SPI.h>
#include "MFRC522.h"
#include <Adafruit_NeoPixel.h>

/* wiring the MFRC522 to ESP8266 (ESP-12)
RST     = GPIO4  - D2 
SDA(SS) = GPIO2  - D4 
MOSI    = GPIO13
MISO    = GPIO12
SCK     = GPIO14
GND     = GND
3.3V    = 3.3V
*/

#define RST_PIN  D8 // RST-PIN für RC522 - RFID - SPI - Modul GPIO15 
#define SS_PIN   D4  // SDA-PIN für RC522 - RFID - SPI - Modul GPIO2 
#define LED_PIN  D1


const char *ssid = "";     // change according to your Network - cannot be longer than 32 characters!
const char *pass = ""; // change according to your Network
const char *host = "membership.hackspace.ca";  // 
const char *nomosApiKey = "";


MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance


Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, LED_PIN, NEO_GRB + NEO_KHZ800);


bool ReconnectToWiFi() {
  
  Serial.print(String(millis()) + " Connecting to " + String(ssid) + String(" ") );  
  WiFi.begin(ssid, pass);
  
  while ((WiFi.status() != WL_CONNECTED) ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  // Check the status. 
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(String(millis()) + " WiFi connected");
    Serial.println(String(millis()) + " IP address: "); Serial.println(WiFi.localIP());
  } else { 
    Serial.println(String(millis()) + " Error: Could not connect to the WiFi. SSID=" + ssid );
  }
}

void setup() {
  Serial.begin(115200);    // Initialize serial communications
  delay(250);
  Serial.println(String(millis()) + " Booting....");
  
  SPI.begin();           // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  ReconnectToWiFi ();
  
  Serial.println(String(millis()) + " Scan for Card and print UID:"); 
}

void LEDCycle() {
  // Set to black 
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));      
  }
  strip.show();

  // Set to red 
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(255, 0, 0));      
      strip.show();
      delay(20);
  }

  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));      
      strip.show();
      delay(20);
  }

  // Set to black 
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));      
  }
  strip.show();
}

void loop() { 

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }
  // Show some details of the PICC (that is: the tag/card)
  Serial.print(millis());
  Serial.print(F(" Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();

  LEDCycle(); 
  CheckNomos("xx:xx:xx:xx:xx:xx:xx"); 
}



  

// Helper routine to dump a byte array as hex values to Serial
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

bool CheckNomos( char * rfid_serial ) {

  Serial.println(String(millis()) + " Attempting to connect to " + host );

  // Connect to the server. 
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println(String(millis()) + " Connection failed");

    ReconnectToWiFi ();
    
    return false;
  }
  Serial.println(String(millis()) + " Connected to "+ String( host ) + "...");

  String url = "/services/web/MemberCardService1.svc/ValidateGenuineCard?key=" + String( rfid_serial ); 
  
  Serial.println(String(millis()) + " Requesting URL: "+ String( url ) );

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "X-Api-Key: " + nomosApiKey + "\r\n" + 
               
               "Connection: close\r\n\r\n");    
  Serial.println(String(millis()) + " Waiting for response...");
  
  unsigned long timeout = millis() + 1000 * 3 ; 
  while(!client.available() ){
    if( millis() > timeout ) {
      break ; 
    }
  }

  while(client.available() ){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println(String(millis()) + " Done.\n\n");
  return true;
}

