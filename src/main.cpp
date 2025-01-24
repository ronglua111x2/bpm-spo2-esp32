#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "heartRate.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <string.h>
#include <stdio.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "time.h"
#include "SPIFFS.h"

#define BUZZER_PIN 5

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MAX30105 particleSensor;

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
// Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
// To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100];  // infrared LED sensor data
uint16_t redBuffer[100]; // red LED sensor data
#else
uint32_t irBuffer[100];  // infrared LED sensor data
uint32_t redBuffer[100]; // red LED sensor data
#endif

static const unsigned char PROGMEM logo2_bmp[] =
    {
        0x03,
        0xC0,
        0xF0,
        0x06,
        0x71,
        0x8C,
        0x0C,
        0x1B,
        0x06,
        0x18,
        0x0E,
        0x02,
        0x10,
        0x0C,
        0x03,
        0x10, // Logo2 and Logo3 are two bmp pictures that display on the OLED if called
        0x04,
        0x01,
        0x10,
        0x04,
        0x01,
        0x10,
        0x40,
        0x01,
        0x10,
        0x40,
        0x01,
        0x10,
        0xC0,
        0x03,
        0x08,
        0x88,
        0x02,
        0x08,
        0xB8,
        0x04,
        0xFF,
        0x37,
        0x08,
        0x01,
        0x30,
        0x18,
        0x01,
        0x90,
        0x30,
        0x00,
        0xC0,
        0x60,
        0x00,
        0x60,
        0xC0,
        0x00,
        0x31,
        0x80,
        0x00,
        0x1B,
        0x00,
        0x00,
        0x0E,
        0x00,
        0x00,
        0x04,
        0x00,
};

static const unsigned char PROGMEM logo3_bmp[] =
    {0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
     0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
     0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
     0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
     0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
     0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
     0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
     0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00};

int32_t bufferLength;  // data length
int32_t spo2;          // SPO2 value
int8_t validSPO2;      // indicator to show if the SPO2 calculation is valid
int32_t heartRate;     // heart rate value
int8_t validHeartRate; // indicator to show if the heart rate calculation is valid

//-------FireBase-------------------
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "Pass"

// Insert Firebase project API Key
#define API_KEY "API_Key"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "url"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

bool isIPSend = false;

// Object for NTP time sync
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7L * 60L * 60L;
const int daylightOffset_sec = 3600;
// Declare a global variable to store the current time
struct tm currentTime;
std::string saveLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "Failed to load time";
  }
  char timeString[30];
  strftime(timeString, sizeof(timeString), "%d/%B/%Y %H:%M:%S", &timeinfo); 
  return timeString;
}

std::string saveLocalTimeDMY()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "Failed to load time";
  }
  char timeString[30];
  strftime(timeString, sizeof(timeString), "%d/%B/%Y", &timeinfo); 
  return timeString;
}


//Setup variable for realtime database.
int currentRecordID, currentUserID, currentUserAge, totalUser;
String currentUserName, newUsername;
bool isInfoSynced, isSessionSynced, isSendTotalUser, hasCreateRequest, hasChangeRequest, isRangeBPMSet, hasDelRecordRequest = false;


//Set up function related to FireBase
template <typename T>
void sendRecordRTDB(String userName, const T &data, const char *dataPath){
  String path = "record/" + String(userName) + "/"+ String(currentRecordID) + String(dataPath);
  if (Firebase.RTDB.set(&fbdo, path, data))
  {
    Serial.println("PASSED");
  }
  else
  {
    Serial.println(fbdo.errorReason());
  }
}
template <typename H>
void sendUserInfoRTDB(const H &data, const char *dataPath){
  String path = "users/" + String(currentUserID) + String(dataPath) ;
  if (Firebase.RTDB.set(&fbdo, path, data))
  {
    Serial.println("PASSED");
  }
  else
  {
    Serial.println(fbdo.errorReason());
  }
}
void syncUserInfo(){
  String namePath = "users/" + String(currentUserID) + "/username";
  String IDPath = "users/" + String(currentUserID) + "/recordID_last";
  String agePath = "users/" + String(currentUserID) + "/age";
  if (!isInfoSynced){
    Serial.println("Syncing Info:");
    if (Firebase.RTDB.getInt(&fbdo, IDPath))
    {
     if (fbdo.dataType() == "int") {
          currentRecordID = fbdo.intData();
          Serial.println(currentRecordID);
        }
    }
    else {
     Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, agePath))
    {
     if (fbdo.dataType() == "int") {
          currentUserAge = fbdo.intData();
          Serial.println(currentUserAge);
        }
    }
    else {
     Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getString(&fbdo, namePath))
    {
      if (fbdo.dataType() == "string") {
          currentUserName = fbdo.stringData();
          Serial.println(currentUserName);
          Serial.println("---------------------------");
          isInfoSynced = true;
       }
    }
    else {
     Serial.println(fbdo.errorReason());
    }
  }
}

template <typename S>
void sendStatusRTDB(const S &data, const char *dataPath)
{
  String path = "status/" + String(dataPath) ;
  if (Firebase.RTDB.set(&fbdo, path, data))
  {
    Serial.println("PASSED");
  }
  else
  {
    Serial.println("FAILED");
  }
}

void syncSession(){
  String sessionPath = "status/lastSessionUID";
  String totalUserPath = "status/totalUser";  
  if (!isSessionSynced){
    Serial.println("Syncing Session:");
    if (Firebase.RTDB.getInt(&fbdo, sessionPath))
    {
     if (fbdo.dataType() == "int") {
          currentUserID = fbdo.intData();
          Serial.println(currentUserID);
        }
    }
    else {
     Serial.println(fbdo.errorReason());
    }
    if (Firebase.RTDB.getInt(&fbdo, totalUserPath))
    {
     if (fbdo.dataType() == "int") {
          totalUser = fbdo.intData();
          Serial.println(totalUser);
          isSessionSynced = true;
        }
    }
    else {
     Serial.println(fbdo.errorReason());
    }    
  }
}

// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);
const char* PARAM_INPUT_1 = "userID";
const char* PARAM_INPUT_2 = "newUsername";
const char* PARAM_INPUT_3 = "newAge";
String handleRoot() {
  String index = SPIFFS.open("/index.html", "r").readString();
  return index;
}

String handleRecord() {
  String index = SPIFFS.open("/record.html", "r").readString();
  return index;
}

String handleUser() {
  String index = SPIFFS.open("/user.html", "r").readString();
  return index;
}

String handleScript() {
  String script = SPIFFS.open("/script.js", "r").readString();
  return script;
}

const char reloadPageJS15s[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Đồ án 2</title>
</head>
<body>
<div id="countdown"></div>
<script>
    var timeleft = 15;
    var downloadTimer = setInterval(function(){
      if(timeleft <= 0){
        clearInterval(downloadTimer);
        document.getElementById("countdown").innerHTML = "Quay trở lại...";
      } else {
        document.getElementById("countdown").innerHTML ="Vui lòng chờ " + timeleft + " giây...";
      }
      timeleft -= 1;
    }, 1000);
    function reloadPage() 
    { 
    location.href ="/"
    }; 
    setTimeout(reloadPage, 16000);
</script>
</body>
</html>
)rawliteral";
const char reloadPageJS7s[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Đồ án 2</title>
</head>
<body>
<div id="countdown"></div>
<script>
    var timeleft = 7;
    var downloadTimer = setInterval(function(){
      if(timeleft <= 0){
        clearInterval(downloadTimer);
        document.getElementById("countdown").innerHTML = "Quay trở lại...";
      } else {
        document.getElementById("countdown").innerHTML ="Vui lòng chờ " + timeleft + " giây...";
      }
      timeleft -= 1;
    }, 1000);
    function reloadPage() 
    { 
    location.href ="/"
    }; 
    setTimeout(reloadPage, 8000);
</script>
</body>
</html>
)rawliteral";
// String reloadPageJS = "<script>function reloadPage() { location.href =\"/\";}; setTimeout(reloadPage, 1000); </script>";
// String reload5sPageJS = "Vui lòng đợi 5 giây...<script>function reloadPage() { location.href =\"/\";}; setTimeout(reloadPage, 5000); </script>";


//Varibles for switch between 2 display
unsigned long previousMillis = 0;
const long interval = 3500;
bool isUserInfoDisplaying = false;

//Funct for display text on SSD screen, would clear display everytime got called.
void screenDisplay(String firstText, String secondText){
    // String text1 = String(firstText);
    // String text2 = String(secondText);
    // display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(30, 10);
    display.println(firstText);
    display.setCursor(30, 20);
    display.println(secondText);
    display.display();
}

//Warning screen display
void warningDisplay(String bpmWarning, String spo2Warning){
  display.clearDisplay();                             // Clear the display
  display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE); // Draw the second picture (bigger heart)
  display.setTextSize(2);                            
  display.setTextColor(WHITE);
  display.setCursor(0, 40);                
  display.print("SPO2:");                
  display.setCursor(56, 8);
  display.println(bpmWarning);
  display.setCursor(56, 40);
  display.println(spo2Warning);                
  display.display();                
}

//Funct for determine normal BPM range according to age
int lowBarBPM, highBarBPM;

void rangeBPM(int low, int high){
  lowBarBPM = low;
  highBarBPM = high;
  isRangeBPMSet = true;
}
//Filter BPM and SPO2
// int filterSensorValue(){
//   if (spo2 > 90 &&  )
// }

void setup()
{
  Serial.begin(115200);
  //Set up LEDC for buzzer
  ledcSetup(5, 5000, 8); // Pin 5, 5 kHz frequency, 8-bit resolution
  ledcAttachPin(5, 5);    // Attach pin 5 to LEDC channel 5
  pinMode(BUZZER_PIN, OUTPUT);
  // Wifi connection and FireBase
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("FireBase signed up!");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Init and get the current time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  std::string timeString = saveLocalTime();
  Serial.println(timeString.c_str());

  //Check mounting SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  else{
  Serial.println("Mount SPIFFS succeeded");
  //---------------------------------------------------------------->>>
  //Setup for WebServer
  //GET route for landing page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", handleRoot()); });
  
  //GET route for user data page
  server.on("/user.html",  HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", handleUser()); });
  
  //GET route for record data page
  server.on("/record.html",  HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", handleRecord()); });
  
  //GET route for script
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/javascript", handleScript()); });
  //--------------------------------------------------------------------<<
  //GET request to change session's currentUserID
  server.on("/changeUID", HTTP_GET, [] (AsyncWebServerRequest *request) { 
    if (request->hasParam(PARAM_INPUT_1)) {
      String userID = request->getParam(PARAM_INPUT_1)->value();
      sscanf(userID.c_str(), "%d", &currentUserID);   
      Serial.println("ID got ");
      Serial.println(userID);
      hasChangeRequest = true;
      //inputParam = PARAM_INPUT_1;
    }
    else {
      Serial.println("Error while updata variable to esp32");
    }
    //Return to landing page after updated id.
    //Request must be first, otherwise could the Server could crash
    // String javascript = "<script>location.href ='/';</script>";
    request->send(200, "text/html", reloadPageJS7s);    
    //Resync after updated
    display.clearDisplay();
    isInfoSynced = isSessionSynced = isRangeBPMSet = false;
  });

  //GET route to create new user
  server.on("/createUser", HTTP_GET, [] (AsyncWebServerRequest *request) {    
    //Get new username
    if (request->hasParam(PARAM_INPUT_2)) {
      newUsername = request->getParam(PARAM_INPUT_2)->value();
      Serial.println(newUsername);        
    }
    else {
      Serial.println("Error while updata variable to esp32");
    }
    //Get age of new username
    if (request->hasParam(PARAM_INPUT_3)) {
      String userAge = request->getParam(PARAM_INPUT_3)->value();
      sscanf(userAge.c_str(), "%d", &currentUserAge);   
      Serial.println("User's age ");
      Serial.println(userAge);
      hasCreateRequest = true;
    }
    else {
      Serial.println("Error while updata variable to esp32");
    }
    // String javascript = "<script>location.href ='/';</script>";
    request->send(200, "text/html", reloadPageJS7s);    
    isInfoSynced = isSessionSynced = isRangeBPMSet = false;
  });

  //GET requet to handle delete Record function
  server.on("/deleteRecord", HTTP_GET, [] (AsyncWebServerRequest *request) {  
    if (request->hasParam("deleteRecord")) {
      String isDeleteRecord = request->getParam("deleteRecord")->value();
      Serial.println(isDeleteRecord);
      String trueString = "confirmed";
      if (isDeleteRecord == trueString){
        request->send(200, "text/html", reloadPageJS7s); 
        isInfoSynced = isSessionSynced = isRangeBPMSet = false;
      }   
      else{
        String javascript = "<script>location.href ='/';</script>";
        request->send(200, "text/html", javascript); 
      }
    }
    else{
      Serial.println("Error while updata variable to esp32");
    }
  });
  
  //Get request to hadle delete All function
  server.on("/deleteAllData", HTTP_GET, [] (AsyncWebServerRequest *request) {
    display.clearDisplay();
    screenDisplay("Change detected","Please wait...");  
    if (request->hasParam("deleteAllData")) {
      String isdeleteAllData = request->getParam("deleteAllData")->value();
      Serial.println(isdeleteAllData);
      String trueString = "confirmed";
      if (isdeleteAllData == trueString){
        request->send(200, "text/html", reloadPageJS15s);    
        //Resync after updated
        isIPSend = isSendTotalUser = false;
        isInfoSynced = isSessionSynced = isRangeBPMSet = false;
      }   
      else{
        Serial.println("User cancel delete request!");
        String javascript = "<script>location.href ='/';</script>";
        request->send(200, "text/html", javascript); 
      }
    }
    else{
      Serial.println("Error while updata variable to esp32");
    }
    // String javascript = "<script>location.href ='/';</script>";
    // request->send(200, "text/html", reloadPageJS15s);    
    //Resync after updated
  });
  
  // Start server
  server.begin();
  Serial.println(F("Server can be connected!"));
  }

  /*---------------------------------------------------------------------------*/
  // Setup for SSD1306
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 failed"));
    for (;;)
      ;
  }
  display.display();
  delay(1000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println(F("Display OK"));
  display.display();
  delay(1000);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println(F("MAX30102 was not found"));
    display.println(F("MAX30102 FAILED"));
    display.display();
    for (;;)
      ;
  }
  display.println(F("Sensor OK"));
  display.display();
  delay(1000);
  Serial.println(F("MAX initialized. Sensor is ready!"));
  display.clearDisplay();
  particleSensor.setup(60, 4, 2, 100, 411, 4096); // Configure sensor with these settings

}


void loop()
{
  IPAddress ip = WiFi.localIP();
  String ipAddress = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  
  // tone(BUZZER_PIN, 1000);
  //Send Local IP address of ESP32/Webserer
  if (!isIPSend){
    sendStatusRTDB(ipAddress,"/ip");
    Serial.println("IP sent!");
    display.clearDisplay();
    for(int i = 3; i > 0; i--){
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(20, 5);
        display.println("Control device at:");                
        display.setCursor(30, 20);
        display.println(ipAddress);
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(60, 40);
        display.println(i);     
        display.display();
        delay(1000);
    }
        for(int i = 3; i > 0; i--){
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(8, 10);
        display.printf(">>Place finger");
        display.setCursor(10, 20);
        display.printf("and Wait for...");
        display.setCursor(10, 30);
        display.printf("a Hearbeat<<");
        display.setTextSize(2);
        display.setTextColor(WHITE);        
        display.setCursor(60, 40);        
        display.println(i);     
        display.display();
        delay(1000);
    }
    tone(5, 1000, 500);
    tone(5, 1000, 500);    
    isIPSend = true;
  }

  if (hasDelRecordRequest == true){
    Serial.print("Delete record of: ");
    Serial.println(currentUserID);
    String recordPath = "record/" + String(currentRecordID);
    Firebase.RTDB.set(&fbdo, recordPath, NULL);
    sendUserInfoRTDB(0, "/recordID_last");
    display.clearDisplay();
    screenDisplay("Deleting record....", "--------");
    delay(500);
    hasDelRecordRequest == false;
  }

  if (hasChangeRequest == true){
    Serial.println("ID Updated");
    Serial.println(currentUserID);
    sendStatusRTDB(currentUserID,"/lastSessionUID");      
    Serial.println("==========");
    display.clearDisplay();
    screenDisplay("Switch user...", "--------");
    delay(500);
    // delay(300);
    // syncSession();
    // syncUserInfo();
    hasChangeRequest = false;
  } 

  if (hasCreateRequest == true){
    Serial.println("Creating user....");
    totalUser++;
    currentUserID = totalUser;        
    
    sendUserInfoRTDB(newUsername, "/username"); 
    sendUserInfoRTDB(currentUserAge, "/age");
    sendUserInfoRTDB(0, "/recordID_last");
    std::string timeStamp = saveLocalTimeDMY();
    sendUserInfoRTDB(timeStamp, "/dateCreated");    
    sendUserInfoRTDB(totalUser, "/id");
    
    sendStatusRTDB(totalUser, "/lastSessionUID");
    sendStatusRTDB(totalUser, "/totalUser");
    Firebase.RTDB.setBool(&fbdo, "status/firstBoot", false);
    Firebase.RTDB.setInt(&fbdo, "record/" + newUsername + "/", 1);
    
    Serial.println("User has been created!");
    Serial.println("======================");
    display.clearDisplay();
    screenDisplay(newUsername, "User valid");
    currentUserName = newUsername;
    delay(500);
    // syncSession();
    // syncUserInfo();
    hasCreateRequest = false;
  }  

  if (Firebase.RTDB.pathExisted(&fbdo, "status/firstBoot") == true && hasChangeRequest == false && hasCreateRequest == false ){
    if(isSessionSynced == false && isInfoSynced == false && isRangeBPMSet == false){
        syncSession();
        syncUserInfo();
        if (currentUserAge == 0){
          rangeBPM(100,205);
        }
        else if(currentUserAge == 1){
          rangeBPM(100, 180);
        }
        else if (currentUserAge == 2){
          rangeBPM(98, 140);     
        }
        else if (currentUserAge >= 3 && currentUserAge <=6 ){
          rangeBPM(80, 120);
        }
        else if (currentUserAge >= 7 && currentUserAge <= 12)
        {
          rangeBPM(75,118);
        }
        else{
          //Age > 13
          rangeBPM(60,100);
        }
        Serial.println("BPM range:");    
        Serial.println(lowBarBPM);
        Serial.println(highBarBPM); 
    }
    else if (isSessionSynced == true && isInfoSynced == true && isRangeBPMSet == true){
      display.clearDisplay();

      bufferLength = 100; // buffer length of 100 stores 4 seconds of samples running at 25sps
      
      long irValue = particleSensor.getIR();
      long sensitivityThreshold = 10000;  //The greater this value, the more sensitive finger sensor.
      if (irValue > sensitivityThreshold){
        currentRecordID++;
        // read the first 100 samples, and determine the signal range
        tone(5, 1000, 500);
        for (byte i = 0; i < bufferLength; i++)
        {
          while (particleSensor.available() == false)
            particleSensor.check(); // Check the sensor for new data

          redBuffer[i] = particleSensor.getRed();
          irBuffer[i] = particleSensor.getIR();
          irValue = particleSensor.getIR();
          if (irValue <= sensitivityThreshold)
          {
            currentRecordID--;
            Serial.println("Finger removed. Exiting measurement loop.");
            display.clearDisplay();
            screenDisplay("Finger removed!", "Stop Measuring");
            delay(300); 
            break;

          }
          particleSensor.nextSample(); // Move to the next sample

          // Calculate the percentage of measurement completed
          float percentage = ((float)(i + 1) / bufferLength) * 100;

          // Display the percentage on the OLED
          display.clearDisplay();
          display.setCursor(10, 10);
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.print("Taking sample.....");
          display.setCursor(10, 25);
          display.print("Hold finger still !");
          display.setCursor(20, 40);
          display.setTextSize(2);
          display.print((int)percentage); // Cast to int for cleaner display
          display.println("%");
          display.display();

          Serial.print(F("red="));
          Serial.print(redBuffer[i], DEC);
          Serial.print(F(", ir="));
          Serial.println(irBuffer[i], DEC);
        }

        // calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

        // Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
        while (1) // Create an infinite loop
        {
          irValue = particleSensor.getIR();
          if (irValue <= sensitivityThreshold){
            Serial.println("Finger removed. Exiting measurement loop.");
            display.clearDisplay();          
            screenDisplay("Finger removed!", "Stop Measuring");
            // std::string timeStamp = saveLocalTime();
            // sendRecordRTDB(currentUserName, timeStamp.c_str(), "/timestamp");               
            // sendRecordRTDB(currentUserName, currentRecordID, "/id");
            // sendRecordRTDB(currentUserName, "Cảm biến lỗi hoặc bỏ ngón tay quá sớm", "/spo2");
            // sendRecordRTDB(currentUserName, "Vui lòng để yên tới khi thấy nhịp tim", "/BPM");
            // sendUserInfoRTDB(currentRecordID, "/recordID_last");
            isInfoSynced = isSessionSynced = isRangeBPMSet = false;
            delay(300);
            break;
          }
          // dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
          for (byte i = 25; i < 100 && irValue > sensitivityThreshold; i++)
          {       
              redBuffer[i - 25] = redBuffer[i];
              irBuffer[i - 25] = irBuffer[i];
          }

          // take 25 sets of samples before calculating the heart rate.
          for (byte i = 75; i < 100 && irValue > sensitivityThreshold; i++)
          {     
              while (particleSensor.available() == false) // do we have new data?
                particleSensor.check();                   // Check the sensor for new data
              // if ( particleSensor.check() == false){
              //   screenDisplay("Out of sample", "Try again");
              //   currentRecordID--;
              //   break;
              // }
              if (particleSensor.available()) {
                  redBuffer[i] = particleSensor.getRed();
                  irBuffer[i] = particleSensor.getIR();
                  particleSensor.nextSample(); 
              } else {
                screenDisplay("Out of sample", "Try again");
                isInfoSynced = isSessionSynced = isRangeBPMSet = false;
                delay(300);
                break;
              }
              Serial.print(F("BPM="));
              Serial.print(heartRate, DEC);

              Serial.print(F(", SPO2="));
              Serial.print(spo2, DEC);

              display.clearDisplay();                             // Clear the display
              display.drawBitmap(5, 5, logo2_bmp, 24, 21, WHITE); // Draw the first bmp picture (little heart)
              display.setTextSize(2);                             // Near it display the average BPM you can display the BPM if you want
              display.setTextColor(WHITE);
              display.setCursor(50,8);
              if (heartRate >= 50 && heartRate <= 220)
              {
                display.print(heartRate);
              }
              else{
                display.print("--");
              }
              display.println("bpm");
              display.setCursor(0,40);
              display.print("SPO2:");
              if (spo2 >= 20 && spo2 <= 100 )
              {
                display.print(spo2);
              }
              else{
                display.print("--");
              }
              display.println("%");              
              // display.setCursor(52, 10);
              // display.print(heartRate);
              // display.println("waiting...");
              // display.setTextSize(2); 
              // display.setCursor(0, 40);
              // display.print("SPO2:");
              // display.print(spo2);
              // display.setTextSize(1);  
              // display.setCursor(52, 40);
              // display.println(" hold...");
              display.display();

              //
              if (irValue <= sensitivityThreshold) {
                Serial.println("Finger removed. Exiting measurement loop.");
                display.clearDisplay();   
                screenDisplay("Finger removed!", "Stop Measuring");
                isInfoSynced = isSessionSynced = isRangeBPMSet = false;
                break;
              }
              if (checkForBeat(irBuffer[i]) == true) // If a heart beat is detected
              {
                tone(5, 1000, 100); // And tone the buzzer for a 100ms you can reduce it it will be better
                std::string timeStamp = saveLocalTime();
                sendRecordRTDB(currentUserName, timeStamp.c_str(), "/timestamp");               
                sendRecordRTDB(currentUserName, currentRecordID, "/id");
                // sendRecordRTDB(currentUserName, spo2, "/spo2");
                // sendRecordRTDB(currentUserName, heartRate, "/BPM");
                sendUserInfoRTDB(currentRecordID, "/recordID_last");    
                display.clearDisplay();                             // Clear the display
                display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE); // Draw the second picture (bigger heart)
                display.setTextSize(2);                             // And still displays the average BPM
                display.setTextColor(WHITE);
                display.setCursor(50,8);
                if (heartRate >= 50 && heartRate <= 220)
                {
                  display.print(heartRate);
                  sendRecordRTDB(currentUserName, heartRate, "/BPM");
                }
                else{
                  display.print("--");
                  sendRecordRTDB(currentUserName, "--", "/BPM");                  
                }
                display.println("bpm");
                display.setCursor(0,40);
                display.print("SPO2:");
                if (spo2 >= 20 && spo2 <= 100 )
                {
                  display.print(spo2);
                  sendRecordRTDB(currentUserName, spo2, "/spo2");                  
                }
                else{
                  display.print("--");
                  sendRecordRTDB(currentUserName, "--", "/spo2");                  
                }
                display.println("%"); 
                display.display();           
                delay(1500);
                if (spo2 < 90 && (heartRate < lowBarBPM | heartRate > highBarBPM)){
                  Serial.println("Warning!");
                  warningDisplay("Bad !","Bad !"); 
                  sendRecordRTDB(currentUserName, "Tệ | Tệ", "/analyzed");             
                  tone(5, 1000, 500);              
                }
                else if (spo2 >= 90 && (heartRate < lowBarBPM | heartRate > highBarBPM)){
                  Serial.println("Warning!");
                  warningDisplay("Bad !","Normal");
                  sendRecordRTDB(currentUserName, "Tệ | Ổn", "/analyzed");                                    
                  tone(5, 1000, 250);
                
                }
                else if(spo2 < 90 && (heartRate >= lowBarBPM && heartRate <= highBarBPM)){
                  Serial.println("Warning!");
                  warningDisplay("Normal","Bad !");      
                  sendRecordRTDB(currentUserName, "Ổn | Tệ", "/analyzed");           
                  tone(5, 1000, 250);
                  
                }
                else if (spo2 >= 90 && (heartRate >= lowBarBPM && heartRate <= highBarBPM)){
                  Serial.println("Normal");
                  warningDisplay("Normal","Normal");    
                  sendRecordRTDB(currentUserName, "Ổn | Ổn", "/analyzed");           
                  tone(5, 1000, 100);              
                }
                delay(1250);    
                break;
              }
              // break;
              Serial.println();

          }
          // After gathering 25 new samples recalculate HR and SP02
          maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);             
        }
      }
      else{     
          // unsigned long currentMillis = millis(); 
          // Serial.println(currentMillis);      
          // if (currentMillis - previousMillis  >= interval) {
          //   previousMillis = currentMillis;  
          //   isUserInfoDisplaying = !isUserInfoDisplaying;
          //   if (isUserInfoDisplaying) {
              display.clearDisplay();
              display.setTextSize(1);
              display.setTextColor(WHITE);
              display.setCursor(20, 10);
              display.printf("ID: ");
              display.println(currentUserID);
              display.setCursor(20, 20);
              display.printf("Name: ");
              display.println(currentUserName);
              display.setCursor(20, 30);
              display.printf("Age: ");
              display.println(currentUserAge);
              display.setCursor(20, 40);
              display.printf("Total record: ");
              display.println(currentRecordID);
              display.display();
          //   } else {
          //     display.clearDisplay();
          //     display.setTextSize(1);
          //     display.setTextColor(WHITE);
          //     display.setCursor(8, 20);
          //     display.printf(">>Place finger");
          //     display.setCursor(10, 30);
          //     display.printf("and wait for");
          //     display.setCursor(10, 40);
          //     display.printf("a hearbeat...");
          //     display.display();
          //   }
          // }      
      }
    }
      
  }
  else if (Firebase.RTDB.pathExisted(&fbdo, "status/firstBoot") == false && hasChangeRequest == false && hasCreateRequest == false){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(30, 10);
    display.println("No user found");
    display.setCursor(23, 20);
    display.println("Create first on ");
    display.setCursor(20,30);
    display.println("WebServer at IP:");
    display.setCursor(30,40);
    display.println(ipAddress);
    display.display();
    totalUser = currentUserID = 0;
    if (!isSendTotalUser){
      sendStatusRTDB(totalUser,"/totalUser");
      isSendTotalUser = true;
    }
  }
}
