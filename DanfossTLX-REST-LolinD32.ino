/*
    Name: DanfossTLX-REST

    Used Libraries - output from verbose compile
    Multiple libraries were found for "WiFi.h" Used:       ...\packages\esp32\hardware\esp32\2.0.3\libraries\WiFi
    Using library WiFi at version 2.0.0 in folder:         ...\packages\esp32\hardware\esp32\2.0.3\libraries\WiFi
    Using library WebServer at version 2.0.0 in folder:    ...\packages\esp32\hardware\esp32\2.0.3\libraries\WebServer
    Using library ArduinoJson at version 6.19.4 in folder: ...\libraries\ArduinoJson
    Using library FS at version 2.0.0 in folder:           ...\packages\esp32\hardware\esp32\2.0.3\libraries\FS

*/

#include <Arduino.h> // Can be commented out, if you face any issues in Arduino IDE
#include "DanfossTLX-RS485.h"
#include "Secrets.h" // #defines of SSID & WIFI_PSWD
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

// Use this, if you want to set a static IP address, remember to uncomment the WiFi.config line in setup()
// This could be moved to a config file, but that's not implemented right now.
// IPAddress staticIP(192, 168, 1, 25); // Set your desired static IP address here
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);

#define BLYNK_PRINT Serial
#define RXD2 16
#define TXD2 17
#define HOST "DanfossTLX-REST-%s" // Change this to whatever you like.

String Program = "DanfossTLX-REST";
String Arduino = "1.18.19";
String CodeDate = "2023-02-07";
String Board = "LOLIN D32 - ESP32 2.0.3";

DanfossTLX MyTLX(RXD2, TXD2);
WebServer server(80);

StaticJsonDocument<1000> jsonDocument;
char jsonbuffer[1000];

// All values from TLX in one json respons without units
void CreateJsonAll(void)
{
  jsonDocument.clear();
  float temp = 1.0;
  for (int i = 0; i < MyTLX.DATA_ENUMS; i++)
  {
    jsonDocument["Product"] = MyTLX.TLX.ProductNumber.c_str();
    jsonDocument["Serial"] = MyTLX.TLX.SerialNumber.c_str();
    jsonDocument["OpModeTxt"] = MyTLX.TLX.OpModeTxt.c_str();
    jsonDocument[MyTLX.TLX.ParName[i]] = MyTLX.MeasString((DanfossTLX::Par_e)i);
  }
  serializeJson(jsonDocument, jsonbuffer);
}

void CreateJson(const char *tag, float value, const char *unit)
{
  jsonDocument.clear();
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, jsonbuffer);
}
void CreateJsonTxt(const char *tag, const char *value)
{
  jsonDocument.clear();
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  serializeJson(jsonDocument, jsonbuffer);
}

// Web page that holds all parameters/measurements in a table
void HandleRoot()
{
  String msg = "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=60><title>Danfoss TLX</title></head><style>tr:nth-child(even) {background-color: #f3f3f3;}</style><body><h1>Danfoss TLX</h1><table><tr><th>Description</th><th>Value</th><th>Unit</th><th>Indeks</th></tr>";
  if (MyTLX.TLX.ProductNumber.c_str()[0] != 0)
  {
    msg += "<tr><td>Product Number</td><td>" + MyTLX.TLX.ProductNumber + "</td><td></td><td></td></tr>";
  }
  if (MyTLX.TLX.SerialNumber.c_str()[0] != 0)
  {
    msg += "<tr><td>Serial Number</td><td>" + MyTLX.TLX.SerialNumber + "</td><td></td><td></td></tr>";
  }
  if (MyTLX.TLX.OpModeTxt.c_str()[0] != 0)
  {
    msg += "<tr><td>Operation mode</td><td>" + MyTLX.TLX.OpModeTxt + "</td><td></td><td></td></tr>";
  }
  for (int i = 0; i < MyTLX.DATA_ENUMS; i++)
  {
    msg += "<tr><td>" + MyTLX.TLX.Name[i] + "</td><td>" + MyTLX.MeasString((DanfossTLX::Par_e)i) + "</td><td>" + MyTLX.TLX.Unit[i] + "</td><td>" + MyTLX.TLX.ParName[i] + "</td></tr>";
  }
  msg += "</body></html>";
  server.send(200, "text/html", msg);
}

// Send json object of requested parameter/value. See DanfossTLX::ParName for valid names
void ReturnTLXMeasValue()
{
  String uri = server.uri();
  DanfossTLX::Par_e ParEnum = MyTLX.DATA_ENUMS;
  bool found = false;
  if (uri.length() > 3 && uri.startsWith("/"))
  {
    uri.remove(0, 1);
    // Serial.println(uri);
    for (int i = 0; i < MyTLX.DATA_ENUMS; i++)
    {
      // Serial.println( uri + "<>" + MyTLX.TLX.ParName[i] );
      uri.toUpperCase();
      String Name = MyTLX.TLX.ParName[i];
      Name.toUpperCase();
      if (uri == Name)
      {
        found = true;
        CreateJson(MyTLX.TLX.ParName[i].c_str(), MyTLX.TLX.Meas[i], MyTLX.TLX.Unit[i].c_str());
        server.send(200, "application/json", jsonbuffer);
        break;
      }
    }
  }
  // Wrong parameter - return txt result for debug
  // http://#IP#/Unkown?Earth=MostlyHarmless&Answer=42
  // Parameter not recognized
  // URI: /Unkown
  // Method: GET
  // Arguments: 2
  //  Earth: MostlyHarmless
  //  Answer: 42

  if (!found)
  {
    String msg = "Parameter not recognized \n\n";
    msg += "URI: ";
    msg += server.uri();
    msg += "\nMethod: ";
    msg += (server.method() == HTTP_GET) ? "GET" : "POST";
    msg += "\nArguments: ";
    msg += server.args();
    msg += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
      msg += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", msg);
  }
}

void setup()
{
  char host[64];
  // Initialize chipID buffer
  char chipID[9]; // Make sure chipID has enough space to store the MAC address
  chipID[8] = '\0';
  // Get the chip ID
  uint8_t mac[6]; // Buffer to hold the MAC address
  esp_err_t err = esp_efuse_mac_get_default(mac);
  if (err != ESP_OK)
  {
    // Handle error
  }
  // Format the chip ID as a hexadecimal string
  sprintf(chipID, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  sprintf(host, HOST, chipID);

  Serial.begin(115200); // Debug console
  Serial.println(CodeDate + Program);
  Serial.println(Arduino);
  Serial.println(Board);
  Serial.println(String("Compile: ") + String(__DATE__) + " " + String(__TIME__));
  Serial.println(String(__FILE__));

  WiFi.mode(WIFI_STA); // Connect to AP
  WiFi.hostname(host);
  // Uncomment to use static IP
  // WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(SECRET_SSID, SECRET_WIFI_PSWD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(5000);
    // Give up and do a reboot
    ESP.restart();
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SECRET_SSID);
  // Set static IP
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HandleRoot);
  server.on("/OpModeTxt", []()
            { CreateJsonTxt("OpModeTxt", MyTLX.TLX.OpModeTxt.c_str());      server.send(200, "application/json", jsonbuffer); });
  server.on("/Product", []()
            { CreateJsonTxt("Product",   MyTLX.TLX.ProductNumber.c_str() ); server.send(200, "application/json", jsonbuffer); });
  server.on("/SerialTxt", []()
            { CreateJsonTxt("Serial",    MyTLX.TLX.SerialNumber.c_str() );  server.send(200, "application/json", jsonbuffer); });
  server.on("/All", []()
            { CreateJsonAll();  server.send(200, "application/json", jsonbuffer); });
  server.onNotFound(ReturnTLXMeasValue);
  ArduinoOTA.setHostname(host);
  ArduinoOTA.begin();
  server.begin();
  Serial.println("HTTP server started\nSetup ended");
}

void loop()
{
  const unsigned long WIFITIME = 30000;
  const unsigned long TIMETLX = 800; // 36 par x 2 times a minute -> ~830ms each (Not to block server.handleClient();
  const unsigned long TIMEPRINT = 60000;
  static unsigned long TimeTLX = millis();
  static unsigned long TimePrint = millis();

  server.handleClient();

  // Wraparround every ~50 days;
  // Should not be neceesary after changer to unsigned long
  if (millis() < TimeTLX)
  {
    TimeTLX = millis();
  }

  if (millis() - TimeTLX > TIMETLX)
  {
    MyTLX.GetParameters();
    // MyTLX.PrintAll();
    TimeTLX = millis();
  }
  if (millis() - TimePrint > TIMEPRINT)
  {
    // MyTLX.PrintAll();
    TimePrint = millis();
  }

  // Test if connected
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Restart Wifi");
    WiFi.disconnect();
    WiFi.begin(SECRET_SSID, SECRET_WIFI_PSWD);
    unsigned long timenow = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - timenow < WIFITIME)
    {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Timeout");
    }
  }
  delay(2);
}