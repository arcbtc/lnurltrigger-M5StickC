

//////////////LOAD LIBRARIES////////////////

#include "FS.h"
#include <WiFiManager.h> 
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "SPIFFS.h"
#include <M5StickC.h>

/////////////////SOME VARIABLES///////////////////

char lnbits_server[40] = "lnbits.com";
char admin_key[300] = "";
char lnbits_description[100] = "lnurltrigger";
char lnbits_amount[500] = "100";
char high_pin[5] = "26";
char time_pin[20] = "200";
char webhook_url[100] = "";
char success_message[100] = "Thanks for the payment!";
char success_url[100] = "";
char static_ip[16] = "10.0.1.56";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";
bool shouldSaveConfig = false;
bool down = false;
bool newLNURL = false;
bool firstCheck = true;
bool buttonA = false;
const char* spiffcontent = "";
String spiffing; 
int new_balance = 0;
int old_balance = 0;

/////////////////////SETUP////////////////////////

void setup()
{
  M5.begin();
  M5.Axp.ScreenBreath(10);
  Serial.begin(115200);
  M5.Lcd.setRotation(3);
  connectingScreen();

  if(USEPORTAL){
   // START PORTAL
   portal();
  }
  else{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
    }
  }
  
}

void connectingScreen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.println("");
  M5.Lcd.println(" CONNECTING");
}

void onlineScreen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.println("");
  M5.Lcd.println(" ONLINE :)");
}

void offlineScreen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.println("");
  M5.Lcd.println(" OFFLINE");
}

void portalScreen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.println("");
  M5.Lcd.println(" PORTAL");
}

void pressAScreen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.println("");
  M5.Lcd.println(" PRESS A");
}
///////////////////MAIN LOOP//////////////////////

void loop() {
  pinMode (atoi(high_pin), OUTPUT);
  digitalWrite(atoi(high_pin), LOW);
  
  getbalance();
  Serial.println(new_balance);
  Serial.println(old_balance);

  if(down){
  offlineScreen();
  getbalance();
  delay(3000);
  }

  if(new_balance > old_balance){
      old_balance = new_balance;
      digitalWrite(atoi(high_pin), HIGH);
      delay(atoi(time_pin));
      digitalWrite(atoi(high_pin), LOW); 
  }
  onlineScreen();
  delay(3000);
}

//////////////////NODE CALLS///////////////////

void getbalance() {
  WiFiClientSecure client;
  client.setInsecure();
  const char *lnbitsserver = lnbits_server;
  const char *adminkey = admin_key;

  if (!client.connect(lnbitsserver, 443)){
    down = true;
    return;   
  }

  String url = "/api/v1/wallet";
  client.print(String("GET ") + url +" HTTP/1.1\r\n" +
                "Host: " + lnbitsserver + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key: "+ invoicekey +" \r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    if (line == "\r") {
      break;
    }
  }
  
  String line = client.readString();

  StaticJsonDocument<500> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    down = true;
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  int balance = doc["balance"];
  if(firstCheck){
    old_balance = balance;
    firstCheck = false;
  }
  new_balance = balance;
  
}

bool makelnurlp()
{
  WiFiClientSecure client;
  client.setInsecure();
  const char *lnbitsserver = lnbits_server;
  const char *adminkey = admin_key;
  const char *lnbitsamount = lnbits_amount;
  const char *lnbitsdescription = lnbits_description;
  const char *webhookurl = webhook_url;
  const char *successmessage = success_message;
  const char *successurl = success_url;

  WiFiManager wm;
  Serial.println("mounting FS...");
  while(!SPIFFS.begin(true)){
    Serial.println("failed to mount FS");
    delay(200);
   }

//CHECK IF RESET IS TRIGGERED/WIPE DATA
   pressAScreen();
  for (int i = 0; i <= 100; i++) {
   if (M5.BtnA.wasReleased())
      {
        buttonA = true;
        M5.update();
        portalScreen();
        File file = SPIFFS.open("/config.txt", FILE_WRITE);
        file.print("placeholder");
        wm.resetSettings();
        i = 100;
      }
      M5.update();
     delay(50);
    }

//MOUNT FS AND READ CONFIG.JSON
  File file = SPIFFS.open("/config.txt");
  
  spiffing = file.readStringUntil('\n');
  spiffcontent = spiffing.c_str();
  DynamicJsonDocument json(1024);
  deserializeJson(json, spiffcontent);
  if(String(spiffcontent) != "placeholder"){
    strcpy(lnbits_server, json["lnbits_server"]);
    strcpy(invoice_key, json["invoice_key"]);
    strcpy(lnbits_amount, json["lnbits_amount"]);
    strcpy(high_pin, json["high_pin"]);
    strcpy(time_pin, json["time_pin"]);
  }
  else{
    portalScreen();
  }

String spiffing = file.readStringUntil('\n');
String spiffcontent = spiffing.c_str();
DynamicJsonDocument json(1024);
deserializeJson(json, spiffcontent);
if (String(spiffcontent) != "placeholder")
{
  strcpy(lnbits_server, json["lnbits_server"]);
  strcpy(lnbits_description, json["lnbits_description"]);
  strcpy(admin_key, json["admin_key"]);
  strcpy(lnbits_amount, json["lnbits_amount"]);
  strcpy(high_pin, json["high_pin"]);
  strcpy(time_pin, json["time_pin"]);
  strcpy(webhook_url, json["webhook_url"]);
  strcpy(success_message, json["success_message"]);
  strcpy(success_url, json["success_url"]);
}

//ADD PARAMS TO WIFIMANAGER
wm.setSaveConfigCallback(saveConfigCallback);

WiFiManagerParameter custom_lnbits_server("server", "LNbits server", lnbits_server, 40);
WiFiManagerParameter custom_lnbits_description("description", "Description", lnbits_description, 200);
WiFiManagerParameter custom_admin_key("admin", "LNbits admin key", admin_key, 300);
WiFiManagerParameter custom_lnbits_amount("amount", "Amount to charge (sats)", lnbits_amount, 10);
WiFiManagerParameter custom_high_pin("high", "Pin to turn on", high_pin, 5);
WiFiManagerParameter custom_time_pin("time", "Time for pin to turn on for (milisecs)", time_pin, 20);
WiFiManagerParameter custom_webhook_url("weburl", "A URL to be called whenever this link receives a payment", webhook_url, 100);
WiFiManagerParameter custom_success_message("message", "Will be shown to the user in his wallet after a successful payment", success_message, 100);
WiFiManagerParameter custom_success_url("sucurl", "Will be shown as a clickable link to the user on payment", success_url, 100);
wm.addParameter(&custom_lnbits_server);
wm.addParameter(&custom_lnbits_description);
wm.addParameter(&custom_admin_key);
wm.addParameter(&custom_lnbits_amount);
wm.addParameter(&custom_high_pin);
wm.addParameter(&custom_time_pin);
wm.addParameter(&custom_webhook_url);
wm.addParameter(&custom_success_message);
wm.addParameter(&custom_success_url);
//IF RESET WAS TRIGGERED, RUN PORTAL AND WRITE FILES

if (!wm.autoConnect("⚡lnurltrigger⚡", "password1"))
{
  Serial.println("failed to connect and hit timeout");
  delay(3000);
  ESP.restart();
  delay(5000);
}
Serial.println("connected :)");
strcpy(lnbits_server, custom_lnbits_server.getValue());
strcpy(lnbits_description, custom_lnbits_description.getValue());
strcpy(admin_key, custom_admin_key.getValue());
strcpy(lnbits_amount, custom_lnbits_amount.getValue());
strcpy(high_pin, custom_high_pin.getValue());
strcpy(time_pin, custom_time_pin.getValue());
strcpy(webhook_url, custom_webhook_url.getValue());
strcpy(success_message, custom_success_message.getValue());
strcpy(success_url, custom_success_url.getValue());
if (shouldSaveConfig)
{
  Serial.println("saving config");
  DynamicJsonDocument json(1024);
  json["lnbits_server"] = lnbits_server;
  json["lnbits_description"] = lnbits_description;
  json["admin_key"] = admin_key;
  json["lnbits_amount"] = lnbits_amount;
  json["high_pin"] = high_pin;
  json["time_pin"] = time_pin;
  json["webhook_url"] = webhook_url;
  json["success_message"] = success_message;
  json["success_url"] = success_url;

  File configFile = SPIFFS.open("/config.txt", "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
  }
  Serial.println("connected :)");
  strcpy(lnbits_server, custom_lnbits_server.getValue());
  strcpy(invoice_key, custom_invoice_key.getValue());
  strcpy(lnbits_amount, custom_lnbits_amount.getValue());
  strcpy(high_pin, custom_high_pin.getValue());
  strcpy(time_pin, custom_time_pin.getValue());
  if (shouldSaveConfig) {
    connectingScreen();
    Serial.println("saving config");
    DynamicJsonDocument json(1024);
    json["lnbits_server"] = lnbits_server;
    json["invoice_key"]   = invoice_key;
    json["lnbits_amount"] = lnbits_amount;
    json["high_pin"]   = high_pin;
    json["time_pin"]   = time_pin;
    
    File configFile = SPIFFS.open("/config.txt", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
      }
      serializeJsonPretty(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      shouldSaveConfig = false;
  }
  connectingScreen();
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
}

void saveConfigCallback () {
  
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
