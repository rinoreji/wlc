
#include <Arduino.h>
#include <WiFi.h>
// #include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <BlynkSimpleEsp32.h>
#include <RunningMedian.h>
#include <FirebaseESP32.h>

#define SAMPLE_COUNT 19
#define host "wlc"

#define FIREBASE_HOST "https://db.firebaseio.com"
#define FIREBASE_AUTH "key"
#define DATA_PATH "/sensor-data"

// #define BLYNK_PRINT Serial
#define BLYNK_TOKEN "token"

float getDistanceToObject();
void sendDataToBlynk(float value);
void startWiFi();
void startMDNS();
void startHttpServer();
void extractAndProcessSensorData();
void uploadDataToRTDB(float value);

RunningMedian samples = RunningMedian(SAMPLE_COUNT);
BlynkTimer timer;
WebServer server(80);

const char *ssid = "home-ssid";
const char *pass = "home-pwd";
const char *ap_ssid = "WLC AP";           // The name of the Wi-Fi network that will be created
const char *ap_password = "password123"; // The password required to connect to it, leave blank for an open network

// defines pins numbers
const int trigPin = 2;
const int echoPin = 5;

FirebaseData firebaseData;
FirebaseJson json1;
int timerId;
unsigned long data_push_interval = 5000L;

/* Style */
String style =
    "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
    "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
    "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
    "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
    "form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
    ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
String loginIndex =
    "<form name=loginForm>"
    "<h1>WLC Login</h1>"
    "<input name=userid placeholder='User ID'> "
    "<input name=pwd placeholder=Password type=Password> "
    "<input type=submit onclick=check(this.form) class=btn value=Login></form>"
    "<script>"
    "function check(form) {"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{window.open('/serverIndex')}"
    "else"
    "{alert('Error Password or Username')}"
    "}"
    "</script>" +
    style;

/* Server Index Page */
String serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
    "<label id='file-input' for='file'>   Choose file...</label>"
    "<input type='submit' class=btn value='Update'>"
    "<br><br>"
    "<div id='prg'></div>"
    "<br><div id='prgbar'><div id='bar'></div></div><br></form>"
    "<script>"
    "function sub(obj){"
    "var fileName = obj.value.split('\\\\');"
    "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
    "};"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    "$.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "$('#bar').css('width',Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!') "
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>" +
    style;

void setup()
{
    Serial.begin(9600); // Starts the serial communication

    pinMode(trigPin, OUTPUT);                                                     // Sets the trigPin as an Output
    pinMode(echoPin, INPUT);                                                      // Sets the echoPin as an Input
    timerId = timer.setInterval(data_push_interval, extractAndProcessSensorData); //timer will run every sec
    Blynk.begin(BLYNK_TOKEN, ssid, pass);

    startWiFi();
    startMDNS();
    startHttpServer();
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop()
{
    server.handleClient();
    Blynk.run();
    timer.run();
}

void startWiFi()
{                                      // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
    WiFi.softAP(ap_ssid, ap_password); // Start the access point
    Serial.printf("Access Point \"%s\" started\n", ap_ssid);

    //WiFi.begin(ssid, pass);

    Serial.println("Connecting");
    while (WiFi.status() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1)
    { // Wait for the Wi-Fi to connect
        delay(500);
        Serial.print('.');
    }
    Serial.println("\r\n");
    if (WiFi.softAPgetStationNum() == 0)
    { // If the ESP is connected to an AP
        Serial.print("Connected to ");
        Serial.println(WiFi.SSID()); // Tell us what network we're connected to
        Serial.print("IP address:\t");
        Serial.print(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
        WiFi.softAPdisconnect(true);
    }
    else
    { // If a station is connected to the ESP SoftAP
        Serial.print("Station connected to AP");
    }
    Serial.println("\r\n");
}

void startHttpServer()
{
    /*return index page which is stored in serverIndex */
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", loginIndex);
    });
    server.on("/serverIndex", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
    });
    /*handling uploading firmware file */
    server.on(
        "/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } });
    server.begin();
}

void startMDNS()
{
    /*use mdns for host name resolution*/
    if (!MDNS.begin(host))
    { //http://esp32.local
        Serial.println("Error setting up MDNS responder!");
        while (1)
        {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");
}

float getDistanceToObject()
{
    samples.clear();
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        Serial.print(".");
        // Clears the trigPin
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);

        // Sets the trigPin on HIGH state for 10 micro seconds
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);

        // Reads the echoPin, returns the sound wave travel time in microseconds
        samples.add(pulseIn(echoPin, HIGH));
        //Serial.printf("Duration: %f - Count: %d\n", duration * 0.034 / 2, samples.getCount());
        delay(10);
    }

    return samples.getMedian() * 0.034 / 2;
}

void sendDataToBlynk(float value)
{
    Blynk.virtualWrite(V1, value);
}

void uploadDataToRTDB(float value)
{
    json1.set("id", "WL");
    json1.set("timestamp", "");
    json1.set("value", value);

    if (Firebase.pushJSON(firebaseData, DATA_PATH, json1))
    {
        Firebase.setTimestamp(firebaseData, firebaseData.dataPath() + "/" + firebaseData.pushName() + "/timestamp");
    }
    else
    {
        Serial.println("Push to RTDB failed. Reason: " + firebaseData.errorReason());
    }
}

void processConfig()
{
    if (Firebase.getInt(firebaseData, "sensor-config/WL/data_push_interval"))
    {
        if (data_push_interval != firebaseData.intData())
        {
            data_push_interval = firebaseData.intData();
            timer.deleteTimer(timerId);
            timerId = timer.setInterval(data_push_interval, extractAndProcessSensorData);
        }
    }
    else
    {
        Serial.println(firebaseData.errorReason());
    }
}

void extractAndProcessSensorData()
{
    float distance = getDistanceToObject();
    Serial.printf("\nDistance to object: %f - # of samples: %d\n", distance, samples.getCount());
    sendDataToBlynk(distance);
    uploadDataToRTDB(distance);
    processConfig();
}