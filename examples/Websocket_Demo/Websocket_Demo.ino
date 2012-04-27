#include <SPI.h>
#include <SC16IS750.h>
#include <WiFly.h>

// Enabe debug tracing to Serial port.
#define DEBUGGING

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1

#include <WebSocket.h>

WiFlyServer server(80);
WebSocket websocketServer;


// Called when a new message from the WebSocket is received
// Looks for a message in this form:
//
// WDPV
//
// Where: W is either 'w' or 'r' - write or read
//        D is either 'd' or 'a' - digital or analog
//        P is a pin #
//        V is optionally the value to apply to the pin
//
// If the message is a read message then the client will be sent
// back a message in the following form:
//
// P:V
//
// Where: P is a pin #
//        V is the value of that pin
void dataReceivedAction(WebSocket &socket, String &dataString) {
  bool isWrite = dataString[0] == 'w';
  bool isDigital = dataString[1] == 'd';
  int pin = dataString[2] - '0';
  int value;

#ifdef DEBUGGING   
  Serial.print(isWrite);
  Serial.print(" ");
  Serial.print(isDigital);
  Serial.print(" ");
  Serial.print(pin);
#endif

  if (isWrite) {
    value = dataString[3] - '0';

#ifdef DEBUGGING
    Serial.print(" ");
    Serial.println(value);
#endif
    
    pinMode(pin, OUTPUT);
   
    if (isDigital) {
      digitalWrite(pin, value);
    } else {
      analogWrite(pin, value);
    }
    
  } else {    
    String result = String(pin);
    
    result += ":";
    
    pinMode(pin, INPUT);
   
    if (isDigital) {
      value = digitalRead(pin);  
    } else {
      value = analogRead(pin);
    }
    
#ifdef DEBUGGING
    Serial.print(" -> ");
    Serial.println(value);
#endif    
    
    result += String(value);
    
    socket.sendData(result);
  }


#ifdef DEBUGGING  
  Serial.println(dataString);
#endif
}

void setup() {
  
#ifdef DEBUGGING  
  Serial.begin(9600);
#endif

  SC16IS750.begin();
  
  WiFly.setUart(&SC16IS750);
  
  WiFly.begin();
  
  // This is for an unsecured network
  // For a WPA1/2 network use auth 3, and in another command send 'set wlan phrase PASSWORD'
  // For a WEP network use auth 2, and in another command send 'set wlan key KEY'
  WiFly.sendCommand(F("set wlan auth 0"));
  WiFly.sendCommand(F("set wlan channel 0"));
  WiFly.sendCommand(F("set ip dhcp 1"));
  
  server.begin();

#ifdef DEBUGGING  
  Serial.println(F("Joining WiFi network..."));
#endif  

  // Here is where you set the network name to join
  if (!WiFly.sendCommand(F("join automata_arduino"), "Associated!", 20000, false)) {
#ifdef DEBUGGING 
    Serial.println(F("Association failed."));
#endif
    while (1) {
      // Hang on failure.
    }
  }
  
  if (!WiFly.waitForResponse("DHCP in", 10000)) {
#ifdef DEBUGGING     
    Serial.println(F("DHCP failed."));
#endif
    while (1) {
      // Hang on failure.
    }
  }

  // This is how you get the local IP as an IPAddress object
#ifdef DEBUGGING 
  Serial.println(WiFly.localIP());
#endif

  websocketServer.addAction(&dataReceivedAction);
}

void loop() {

  WiFlyClient client = server.available();
  
  // This delay is needed to let the WiFly respond properly
  delay(100);
  
  websocketServer.connectionRequest(client);
}
