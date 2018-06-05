
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

PN532_SPI pn532spi(SPI, 5);
PN532 nfc(pn532spi);

#define DEBUG_MODE false

#define UNLOCKING_TIME_DURATION 4000
#define UNLOCKING_RELAY_PIN 4

uint8_t familyUID[][4] = { 
  {123, 154, 132, 23},
  {32, 23, 133, 195},
  {31, 123, 3, 195},
  {13, 26, 31, 54},
  {19, 178, 191, 44},
  {65, 17, 132, 15}
};

int familyMembers = 6;

char names[][10] = {
  {"Gljiva"},
  {"Domagoj"},
  {"Nana"},
  {"Jasna"},
  {"Tomislav"},
  {"Mravac"}
};

String keys[][6] = {
  {"712381"},
  {"131231"},
  {"423423"},
  {"234232"},
  {"234223"},
  {"222342"}
};



ESP8266WiFiMulti WiFiMulti;
const char* ssid = "ssid";
const char* password = "pass";

ESP8266WebServer server(999);

void setup(void) {
  if (DEBUG_MODE) Serial.begin(115200);
  if (DEBUG_MODE) Serial.println("Smart Door is born..");

  pinMode(UNLOCKING_RELAY_PIN, OUTPUT);
  digitalWrite(UNLOCKING_RELAY_PIN, LOW);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (DEBUG_MODE) Serial.print(".");
  }
  if (DEBUG_MODE) Serial.println("");
  if (DEBUG_MODE) Serial.print("Connected to ");
  if (DEBUG_MODE) Serial.println(ssid);
  if (DEBUG_MODE) Serial.print("IP address: ");
  if (DEBUG_MODE) Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    if (DEBUG_MODE) Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.onNotFound(handleRoot);

  server.begin();
  if (DEBUG_MODE) Serial.println("HTTP server started");
  
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    if (DEBUG_MODE) Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();
}

void loop(void) {
  server.handleClient();
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success) {
    if (DEBUG_MODE) Serial.println("Found a card!");
    if (DEBUG_MODE) nfc.PrintHex(uid, uidLength);
    bool auth = false;
    int user = 0;

    for (int person = 0; person < familyMembers; person++) {

      bool is = true;

      for (int i = 0; i < 4; i++)
        if (uid[i] != familyUID[person][i]) is = false;

      if (is) {
        auth = true;
        user = person;
        break;
      }
    }

    if (auth) {
      if (DEBUG_MODE) Serial.print("User ");
      if (DEBUG_MODE) Serial.print(names[user]);
      if (DEBUG_MODE) Serial.println(" authenticated");
      UnlockDoor(UNLOCKING_TIME_DURATION);
    }

    else if (DEBUG_MODE) Serial.println("not authenticated");

    delay(1000);
  }
  else if (DEBUG_MODE) Serial.println("Timed out waiting for a card");
}

void handleRoot() {

  if (server.hasArg("pin")) {
    if (DEBUG_MODE) Serial.println("User has enterd pin..");
    if (DEBUG_MODE) Serial.println(server.arg("pin"));

    for (int i = 0; i < familyMembers; i++ ) {

      if (keys[i][0] == server.arg("pin")) {
        if (DEBUG_MODE) Serial.print("Found ");
        if (DEBUG_MODE) Serial.println(names[i]);
        UnlockDoor(10);

        String webPage;
        webPage += "<!doctype html>";
        webPage += "<html lang='en'>";
        webPage += "<head>";
        webPage += "<meta charset='utf-8'>";
        webPage += "<meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>";
        webPage += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>";
        webPage += "<title>Pametna Vrata</title>";
        webPage += "<style>";
        webPage += ".col {margin: 0 5px;}";
        webPage += "#inputPin {";
        webPage += "text-align:  center;";
        webPage += "font-size: 40px;";
        webPage += "width:  200px;";
        webPage += "letter-spacing: 15px;}";
        webPage += "#inputForm {";
        webPage += "max-width: 300px;";
        webPage += "margin: 10% auto;}";
        webPage += "form .form-group button,";
        webPage += "form .form-group input{ ";
        webPage += "min-height: 50px; ";
        webPage += "font-size: 40px;}";
        webPage += "form .form-group button:active,";
        webPage += "form .form-group button:focus{";
        webPage += "background-color: #fff;";
        webPage += "color: green; }";
        webPage += "p { width: 100%; }";
        webPage += "p.title { font-size: 26px; }";
        webPage += "</style>";
        webPage += "</head>";
        webPage += "<body>";
        webPage += "<div class='container'>";
        webPage += "<div class='row'>";
        webPage += "<form id='inputForm' action='/' method='post' class='col'>";
        webPage += "<div class='form-group row alert alert-success'>";
        webPage += "<p class='title'><strong>";
        webPage += names[i];
        webPage += "</strong></p>";
        webPage += "<p class='desc'>Vrata su se otključala</p></div>";
        webPage += "<div class='form-group row'>";
        webPage += "<input id='inputPin' class='col' type='text' name='pin' placeholder='Pin'>";
        webPage += "</div>";
        webPage += "<div class='form-group row'>";
        webPage += "<button type='button' onclick='ButtonClick(1);' class='col btn btn-outline-success'>1</button>";
        webPage += "<button type='button' onclick='ButtonClick(2);' class='col btn btn-outline-success'>2</button>";
        webPage += "<button type='button' onclick='ButtonClick(3);' class='col btn btn-outline-success'>3</button>";
        webPage += "</div>";
        webPage += "<div class='form-group row'>";
        webPage += "<button type='button' onclick='ButtonClick(4);' class='col btn btn-outline-success'>4</button>";
        webPage += "<button type='button' onclick='ButtonClick(5);' class='col btn btn-outline-success'>5</button>";
        webPage += "<button type='button' onclick='ButtonClick(6);' class='col btn btn-outline-success'>6</button>";
        webPage += "</div>";
        webPage += "<div class='form-group row'>";
        webPage += "<button type='button' onclick='ButtonClick(7);' class='col btn btn-outline-success'>7</button>";
        webPage += "<button type='button' onclick='ButtonClick(8);' class='col btn btn-outline-success'>8</button>";
        webPage += "<button type='button' onclick='ButtonClick(9);' class='col btn btn-outline-success'>9</button>";
        webPage += "</div>";
        webPage += "<div class='form-group row'>";
        webPage += "<input type='reset' class='col btn btn-success' value='*'>";
        webPage += "<button type='button' onclick='ButtonClick(0);' class='col btn btn-outline-success'>0</button>";
        webPage += "<input type='submit' class='col btn btn-success' value='#'>";
        webPage += "</div>";
        webPage += "</form>";
        webPage += "</div>";
        webPage += "</div>";
        webPage += "<script>";
        webPage += "function ButtonClick(btnId){";
        webPage += "var input = document.getElementById('inputPin');";
        webPage += "if (input.value.length < 6) input.value = input.value + btnId;}";
        webPage += "</script>";
        //  webPage += "<script src='https://code.jquery.com/jquery-3.2.1.slim.min.js' integrity='sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN' crossorigin='anonymous'></script>";
        //  webPage += "<script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js' integrity='sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q' crossorigin='anonymous'></script>";
        //  webPage += "<script src='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js' integrity='sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl' crossorigin='anonymous'></script>";
        webPage += "</body>";
        webPage += "</html>";
  
        //  command = server.uri().substring(1).toInt();
  
        server.send(200, "text/html", webPage);
      }
    }
  }

  String webPage;
  webPage += "<!doctype html>";
  webPage += "<html lang='en'>";
  webPage += "<head>";
  webPage += "<meta charset='utf-8'>";
  webPage += "<meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>";
  webPage += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>";
  webPage += "<title>Pametna Vrata</title>";
  webPage += "<style>";
  webPage += ".col {margin: 0 5px;}";
  webPage += "#inputPin {";
  webPage += "text-align:  center;";
  webPage += "font-size: 40px;";
  webPage += "width:  200px;";
  webPage += "letter-spacing: 15px;}";
  webPage += "#inputForm {";
  webPage += "max-width: 300px;";
  webPage += "margin: 10% auto;}";
  webPage += "form .form-group button,";
  webPage += "form .form-group input{ ";
  webPage += "min-height: 50px; ";
  webPage += "font-size: 40px;}";
  webPage += "form .form-group button:active,";
  webPage += "form .form-group button:focus{";
  webPage += "background-color: #fff;";
  webPage += "color: green; }";
  webPage += "p { width: 100%; }";
  webPage += "p.title { font-size: 26px; }";
  webPage += "</style>";
  webPage += "</head>";
  webPage += "<body>";
  webPage += "<div class='container'>";
  webPage += "<div class='row'>";
  webPage += "<form id='inputForm' action='/' method='post' class='col'>";
  webPage += "<div class='form-group row'>";
  webPage += "<input id='inputPin' class='col' type='text' name='pin' placeholder='Pin'>";
  webPage += "</div>";
  webPage += "<div class='form-group row'>";
  webPage += "<button type='button' onclick='ButtonClick(1);' class='col btn btn-outline-success'>1</button>";
  webPage += "<button type='button' onclick='ButtonClick(2);' class='col btn btn-outline-success'>2</button>";
  webPage += "<button type='button' onclick='ButtonClick(3);' class='col btn btn-outline-success'>3</button>";
  webPage += "</div>";
  webPage += "<div class='form-group row'>";
  webPage += "<button type='button' onclick='ButtonClick(4);' class='col btn btn-outline-success'>4</button>";
  webPage += "<button type='button' onclick='ButtonClick(5);' class='col btn btn-outline-success'>5</button>";
  webPage += "<button type='button' onclick='ButtonClick(6);' class='col btn btn-outline-success'>6</button>";
  webPage += "</div>";
  webPage += "<div class='form-group row'>";
  webPage += "<button type='button' onclick='ButtonClick(7);' class='col btn btn-outline-success'>7</button>";
  webPage += "<button type='button' onclick='ButtonClick(8);' class='col btn btn-outline-success'>8</button>";
  webPage += "<button type='button' onclick='ButtonClick(9);' class='col btn btn-outline-success'>9</button>";
  webPage += "</div>";
  webPage += "<div class='form-group row'>";
  webPage += "<input type='reset' class='col btn btn-success' value='*'>";
  webPage += "<button type='button' onclick='ButtonClick(0);' class='col btn btn-outline-success'>0</button>";
  webPage += "<input type='submit' class='col btn btn-success' value='#'>";
  webPage += "</div>";
  webPage += "</form>";
  webPage += "</div>";
  webPage += "</div>";
  webPage += "<script>";
  webPage += "function ButtonClick(btnId){";
  webPage += "var input = document.getElementById('inputPin');";
  webPage += "if (input.value.length < 6) input.value = input.value + btnId;}";
  webPage += "</script>";
  //  webPage += "<script src='https://code.jquery.com/jquery-3.2.1.slim.min.js' integrity='sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN' crossorigin='anonymous'></script>";
  //  webPage += "<script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js' integrity='sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q' crossorigin='anonymous'></script>";
  //  webPage += "<script src='https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js' integrity='sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl' crossorigin='anonymous'></script>";
  webPage += "</body>";
  webPage += "</html>";

  //  command = server.uri().substring(1).toInt();

  server.send(200, "text/html", webPage);

}

void UnlockDoor(int duration) {
  digitalWrite(UNLOCKING_RELAY_PIN, HIGH);
  if (DEBUG_MODE) Serial.println("Unlocking the door.");
  delay(duration);
  digitalWrite(UNLOCKING_RELAY_PIN, LOW);
  if (DEBUG_MODE) Serial.println("Locking the door");
}
