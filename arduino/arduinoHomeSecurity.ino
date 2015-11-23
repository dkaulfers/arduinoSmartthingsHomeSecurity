// Monitors 3 door switches and 1 motion sensor connected to 4 digital pins.  It communicates
// with the smartthings hub on the local lan.
// Combined client and server, based on code posted by zoomkat on the arduino forums
// Author: Charles Schwer

#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xB6, 0xA7, 0xE1, 0xDE, 0x9A, 0xCD }; //physical mac address, make sure it is unique
IPAddress ip(192,168,1,22); // arduino ip in lan
const unsigned int serverPort = 8090; // port to run the arduino server on

// Smartthings hub information
IPAddress hubIp(192,168,1,43); // smartthings hub ip
const unsigned int hubPort = 39500; // smartthings hub port

// pins that sensors are hooked to, 2 = front door, 3 = back door, 5 = side door, 6 = motion sensor
byte sensors[] = { 2, 3, 5, 6 };
#define NUMSENSORS sizeof(sensors)
byte oldSensorState[NUMSENSORS], currentSensorState[NUMSENSORS];

EthernetServer server(serverPort); //server
EthernetClient client; //client
String readString;

//////////////////////

void setup(){
  for(byte i=0; i<NUMSENSORS;i++) {
    pinMode(sensors[i], INPUT_PULLUP);
  }

  // give the ethernet module time to boot up:
  delay(1000);
  Ethernet.begin(mac, ip);
  server.begin();
}

void loop() {
  if(sensorChanged()) {
    if(sendNotify()) {
      //send another request just to be safe    
      //Added this hack because sometimes the request doesn’t show up at the smartthings hub 
      //TODO: figure out what is happening with the request on the network
      delay(10);
      sendNotify();
      // update old sensor state after we’ve sent the notify
      for(byte i=0; i<NUMSENSORS;i++) {
        oldSensorState[i] = currentSensorState[i];
      }
    }
  }

  // Handle any incoming requests
  EthernetClient client = server.available();
  if (client) {
    handleRequest(client);
  }
}

boolean sensorChanged() {
  boolean retVal = false;
  for(byte i=0; i<NUMSENSORS;i++) {
    currentSensorState[i] = digitalRead(sensors[i]);
    if(currentSensorState[i] != oldSensorState[i]) {
      // make sure we didn’t get a false reading
      delay(10);
      currentSensorState[i] = digitalRead(sensors[i]);
      if(currentSensorState[i] != oldSensorState[i]) {
        retVal = true;
      }
    }
  }
  return retVal;
}

// send response to client for a request for status
void handleRequest(EthernetClient client)
{
  boolean currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      //read char by char HTTP request
      if (readString.length() < 100) {
        //store characters to string
        readString += c;
      }
      if (c == '\n' && currentLineIsBlank) {
        //now output HTML data header
        if(readString.substring(readString.indexOf('/'), readString.indexOf('/')+10) == "/getstatus") {
          client.println("HTTP/1.1 200 OK"); //send new page
          sendJSONData(client);
        } else {
          client.println(F("HTTP/1.1 204 No Content"));
          client.println();
          client.println();
        }
        break;
      }
      if (c == '\n') {
        // you're starting a new line
        currentLineIsBlank = true;
      } else if (c != '\r') {
        // you've gotten a character on the current line
        currentLineIsBlank = false;
      }
    }
  }
  readString = "";

  delay(1);
  //stopping client
  client.stop();
}

// send data 
int sendNotify() //client function to send/receieve POST data.
{
  int returnStatus = 1;
  if (client.connect(hubIp, hubPort)) {
    client.println(F("POST / HTTP/1.1"));
    client.print(F("HOST: "));
    client.print(hubIp);
    client.print(F(":"));
    client.println(hubPort);
    sendJSONData(client);
  }
  else {
    //connection failed");
    returnStatus = 0;
  }

  // read any data returned from the POST
  while(client.connected() && !client.available()) delay(1); //waits for data
  while (client.connected() || client.available()) { //connected or data available
    char c = client.read();
  }

  delay(1);
  client.stop();
  return returnStatus;
}

// send json data to client connection
void sendJSONData(EthernetClient client) {
  client.println(F("CONTENT-TYPE: application/json"));
  client.println(F("CONTENT-LENGTH: 140"));
  client.println();
  client.print("{\"house\":{\"door\":[{\"name\":\"front\",\"status\":");
  client.print(currentSensorState[0]);
  client.print("},{\"name\":\"back\",\"status\":");
  client.print(currentSensorState[1]);
  client.print("},{\"name\":\"side\",\"status\":");
  client.print(currentSensorState[2]);
  client.print("}],\"motion\":[{\"name\":\"hall\",\"status\":");
  client.print(currentSensorState[3]);
  client.println("}]}}");
}
