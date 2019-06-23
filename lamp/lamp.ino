#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <ESP8266HTTPClient.h>
ESP8266WebServer server(80);
HTTPClient http;
String stainfo[2] = {"",""};
String gethey = "http://us-central1-lampfunction.cloudfunctions.net/hey";
String getareyoucallingme = "http://us-central1-lampfunction.cloudfunctions.net/areyoucallingme";
String apinfo[2] = {"apmode","123456789"};
bool wasconnected = 0, apflag = 0,lookfor2 = 0,error = 1;
int lamplight[2] = {2,HIGH};
String jsonMessage(String st, String msg){
  String json = "{\"status\":" + st + ",\"message\":\"" + msg + "\"}";
  return json;
}
bool getWiFiParams(){
  if(server.arg("username") == ""||server.arg("password") == "")
    return 0;
  else{
    stainfo[0] = server.arg("username");
    stainfo[1] = server.arg("password");
    return 1;
  }
}
bool ifconnect(){
  int numnet = WiFi.scanNetworks(),i;
  delay(100);
  for(i = 0;i<numnet;i++)
    if(stainfo[0] == WiFi.SSID(i)){
      Serial.println("SSID matched");
      return 1;
    }
  if(i==numnet){
    Serial.println("Unable to find the given network");
    WiFi.disconnect();
    delay(200);
    return 0;
  }
}
void WiFiFromFile(char *fname){
  File f = SPIFFS.open(fname,"r");
  int j=0;
  stainfo[0] = "";
  stainfo[1] = "";
  if(!f){
    Serial.print("file open failed for reading");
  }
  else{
    for(int i=0;i<f.size();i++) //Read upto complete file size
    { 
      char dum = (char)f.read();
      delay(100);
      if (dum == '\n'){
        j++;
        continue;
      }
      stainfo[j] = stainfo[j]+(String)dum;
    }
  }
  f.close();
}
void startStation(){
  
  char* stafilename = "/stationinfo.txt";
  
  if(!SPIFFS.exists(stafilename)){
    Serial.println("Station info not found");
  }
  else{
    //Serial.println("executing get wifi info in station ");
    delay(100);
    WiFiFromFile(stafilename);
    wasconnected = 1;
    if(ifconnect()){
      connectToWifi();
    }
  }
}
void handleRoot(){
    String error = "",ipadd = "",st = "200";
    int wifi = WiFi.status();
    if(wifi == WL_CONNECTED){
      IPAddress ip = WiFi.localIP();
      ipadd = (String)ip[0] + '.' + (String)ip[1]+ '.' + (String)ip[2]+ '.' + (String)ip[3];
    }
    String json = "{\"status\":" + st + 
                  ",\"result\":{\"error\":\"" + error + 
                         "\",\"wifi_connected\":" + wifi + 
                         ",\"stassid\":\"" + stainfo[0] + 
                         "\",\"apssid\":\"" + apinfo[0]  + 
                         "\",\"URL\":\"" + ipadd + 
                         "}}";
    Serial.println(json);
    server.send(200, "application/json", json);
}
void handlestation(){
  
  char* fname = "/stationinfo.txt";
  if(!getWiFiParams()){
    server.send(200,"text/json",jsonMessage("401","Invalid Key(s) in Key-Value pair(s)"));
  }
  else{
    delay(100);
    Serial.println("Connection information changed");
    wasconnected = 0;
    if(ifconnect()){
      if(connectToWifi()){
        char* fname = "/stationinfo.txt";
        server.send(200,"text/json",jsonMessage("200","Connection Successfully"));
        WiFiInFile(fname);
      }
      else{
        if(!apflag)
          connectToAP();
        server.send(200,"text/json",jsonMessage("401","Failed to connect"));
      }
    }
    else{
      if(!apflag)
        connectToAP();
        server.send(200,"text/json",jsonMessage("401","Unable to find the given network"));
    }
  } 
}
void handleFormatSPIFFS(){
  
  int i = SPIFFS.format();
  delay(200);
  String ft = "Formatted = ";
  ft+= (String)i;
  server.send(200,"text/json",jsonMessage("200",ft));

}
bool WiFiInFile(char* fname){
    File f = SPIFFS.open(fname,"w");
    if(!f){
      Serial.print("file open failed for writing");
      server.send(200,"text/json",jsonMessage("402","Could not save new info to file"));
      f.close();
      return 0;
    }
    else{
      for(int i=0;i<stainfo[0].length();i++){
      //Serial.print((char)info[0][i]);
        f.print((char)stainfo[0][i]);  
      }
    //Serial.println("");
      f.print('\n');
    
      for(int i=0;i<stainfo[1].length();i++){
      //Serial.print((char)info[1][i]);
        f.print((char)stainfo[1][i]); 
      }
    //Serial.println("");
      f.print('\n');
      f.close();
      return 1;
    }
}
bool reconnectToWifi(){

  WiFi.begin(stainfo[0],stainfo[1]);
  Serial.println("Reconnecting to \"" + stainfo[0] + "\"...");
  int num=0;
  while(WiFi.status()!=WL_CONNECTED&&num<=25){
    delay(300);
    Serial.print('.');
    num++;
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("");
    Serial.println("Successfully connected");
    Serial.println("Server started");
     //Print the IP address
    Serial.print("Use this URL to connect: ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
    return 1;
  }
  else{
    Serial.println("Failed to connect");
    return 0;
  }
}
bool connectToWifi(){
  WiFi.begin(stainfo[0],stainfo[1]);
  Serial.println("Connecting to \"" + stainfo[0] + "\"...");
  if(WiFi.waitForConnectResult() == WL_CONNECTED){
    Serial.println("");
    Serial.println("Successfully connected");
     //Print the IP address
    Serial.print("Use this URL to connect: ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
    wasconnected = 1;
    return 1;
  }
  else{
    Serial.println("Failed to connect");
    return 0;
  }
}
void blinkit(int times,int ondelay,int offdelay){
  for(int i =1;i<times;i++){
    digitalWrite(2,LOW);
    delay(ondelay);
    digitalWrite(2,HIGH);
    delay(offdelay);
  }
}
void connectToAP(){
    WiFi.softAP(apinfo[0],apinfo[1]);
    Serial.println("Hotspot created:-\nSSID - \"" + apinfo[0] + "\"\nPassword - \"" + apinfo[1] + "\"");
    Serial.println("IP");
    Serial.println(WiFi.softAPIP());
    apflag = 1;
}
int sendRequestAny(String getData,String &payload){
  Serial.println(getData);
  http.begin(getData);
  int httpCode = http.GET();
  payload = http.getString();
  Serial.println(httpCode);
  Serial.println(payload);
  http.end();
  return httpCode;
}
void setup(){
    Serial.begin(115200);
    delay(10);
    pinMode(lamplight[0],OUTPUT);
    digitalWrite(lamplight[0],lamplight[1]);
    WiFi.mode(WIFI_AP_STA);
    digitalWrite(2, HIGH);
    delay(20);
    if(SPIFFS.begin())
      Serial.println("SPIFFS Initialize....ok");
    else 
      Serial.println("SPIFFS Initialization...failed");
    delay(100);
    connectToAP();
    startStation();
    server.on("/", handleRoot);
    server.on("/station", handlestation);
    server.on("/formatspiffs",handleFormatSPIFFS);
    server.begin();
}
void loop(){
  unsigned char val = Serial.read();
  int httpCode;
  String payload = "",getData;
  if(WiFi.status() == WL_CONNECTED){
    if(val == 'a'){
      if(lamplight[1] == HIGH){
        digitalWrite(lamplight[0],lamplight[1]);
        getData = gethey + "?state=1";
        httpCode = sendRequestAny(getData,payload);
        if(httpCode !=200 && payload != "200"){
          blinkit(3,250,75);
          error = 1;
        }
        else{
          lookfor2 = 1;
          error = 0;
        }
        lamplight[1] = LOW;
        digitalWrite(lamplight[0],lamplight[1]);
      }
      else{
        if(!lookfor2){
          getData = gethey + "?state=2";
          httpCode = sendRequestAny(getData,payload);
          if(httpCode !=200 && payload != "200"){
            blinkit(3,250,75);
            error = 1;
          }
          else{
            blinkit(5,1000,150);
            error = 0;
            lookfor2 = 0;
          }
          lamplight[1] = LOW;
          digitalWrite(lamplight[0],lamplight[1]);
        }
        else{
          digitalWrite(lamplight[0],lamplight[1]);
          getData = gethey + "?state=0";
          httpCode = sendRequestAny(getData,payload);
          if(httpCode !=200 && payload != "200"){
            blinkit(3,250,75);
            error = 1;
          }
          else
            error = 0;
          lookfor2 = 0;
          lamplight[1] = HIGH;
          digitalWrite(lamplight[0],lamplight[1]);
        }
      }
    }
    else if(!error){
        httpCode = sendRequestAny(getareyoucallingme,payload);
        if(payload == "0" && lamplight[1] == LOW){
          lamplight[1] = HIGH;
          digitalWrite(lamplight[0], lamplight[1]);
        }
        else if(payload == "1" && lamplight[1] == HIGH){
          lamplight[1] = LOW;
          digitalWrite(lamplight[0],lamplight[1]);
        }
        else if(payload == "2" && lookfor2 && lamplight[1] == LOW){
          blinkit(5,1000,150);
          getData = gethey + "?state=1";
          httpCode = sendRequestAny(getData,payload);
          if(httpCode !=200 && payload != "200"){
            blinkit(3,250,75);
            error = 1;
          }
          else
            error = 0;
          digitalWrite(lamplight[0],lamplight[1]);
        }
        delay(2800);
    }
  }
  else{
    if(val == 'a'){
      blinkit(3,250,75);
      if(lamplight[1] == HIGH){
        lamplight[1] = LOW;
        digitalWrite(lamplight[0],lamplight[1]); 
      }
      else{
        lamplight[1] = HIGH;
        digitalWrite(lamplight[0],lamplight[1]); 
      }
    }
    if(wasconnected&&WiFi.status()!= WL_CONNECTED){
      if(ifconnect())
        if(!reconnectToWifi()&&!apflag)
          connectToAP();
      delay(2800);
    }
  }
  server.handleClient();
}
