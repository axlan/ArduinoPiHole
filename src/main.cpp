

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#include "config.h"

ESP8266WiFiMulti WiFiMulti;




#ifdef DEBUG_PI_HOLE
#ifdef DEBUG_PI_HOLE_PORT
#define DEBUG_PI_HOLE_PRINT(fmt, ...) DEBUG_PI_HOLE_PORT.printf_P( (PGM_P)PSTR(fmt), ## __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_PI_HOLE_PRINT
#define DEBUG_PI_HOLE_PRINT(...) do { (void)0; } while (0)
#endif

class PiHoleCtrl {

public:
  PiHoleCtrl(const String &server_address, const String &persistent_key):
    _index_url("http://" + server_address + "/admin/index.php"),
    _group_url("http://" + server_address + "/admin/scripts/pi-hole/php/groups.php"),
    _persistent_key_cookie("persistentlogin=" + persistent_key + ";") {}

  bool enable_blacklist(WiFiClient &client, const String &name, bool is_enabled) {
    if (_token.length() == 0) {
      if (!get_token(client)) {
        return false;
      } 
    }
    return true;
  }

  bool get_blacklist_group(WiFiClient &client) {
    if (_token.length() == 0) {
      if (!get_token(client)) {
        return false;
      } 
    }
    String data = GET_BLACKLISTS + _token;
    HTTPClient http;
    DEBUG_PI_HOLE_PRINT("[PI] get_blacklist_group");
    if (make_req(client, http, Method::POST, _group_url, &data)) {
      DEBUG_PI_HOLE_PRINT("[PI] out: %s\n", http.getString().c_str());
      http.end();
    }
    return false;
  }
    

private:

  enum Method { GET, POST };

  bool make_req(WiFiClient &client, HTTPClient &http, Method method, const String &url, String *payload=nullptr) {
    
    if (!http.begin(client, url)) {
      DEBUG_PI_HOLE_PRINT("[PI} Unable to connect\n");
      return false;
    }
    
    if (_php_session_cookie.length() > 0) {
      http.addHeader("Cookie", _persistent_key_cookie + " " + _php_session_cookie);
    } else {
      http.addHeader("Cookie", _persistent_key_cookie);
    }
    http.addHeader("Content-type", "application/x-www-form-urlencoded; charset=UTF-8");

    const char * headers[] = {"Set-Cookie"};
    DEBUG_PI_HOLE_PRINT("[PI] SEND...");
    http.collectHeaders(headers, 1);
    int httpCode;
    // start connection and send HTTP header
    switch(method)
    {
        case GET:
          httpCode = http.GET();
          break;
        case POST:
          httpCode = http.POST(*payload);
          break;
        default:
          httpCode = -1;
          break;
    }

    // HTTP header has been send and Server response header has been handled
    // httpCode will be negative on error
    if (httpCode > 0) {
      DEBUG_PI_HOLE_PRINT("[PI] size: %d...", http.getSize());
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        return true;
      } else {
        DEBUG_PI_HOLE_PRINT("[PI] bad code: %d\n", httpCode);
      }
    } else {
      DEBUG_PI_HOLE_PRINT("[PI] SEND... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    return false;
  }

  bool get_token(WiFiClient &client){
    HTTPClient http;
    DEBUG_PI_HOLE_PRINT("[PI] get_token\n");
    if (make_req(client, http, Method::GET, _index_url)) {
      WiFiClient* payload_client = http.getStreamPtr();
      if (http.hasHeader("Set-Cookie")) {
        //PHPSESSID=f0ahlhjddv4jrg2v4i400pe8c2; path=/,persistentlogin=35ca35cbae1861dfad63fe000578de08e4409c5450f7d6310ad7e4c834a4fce7; expires=Fri, 28-Aug-2020 23:34:02 GMT; Max-Age=604800
        if (http.header("Set-Cookie").startsWith("PHPSESSID=")) {
            _php_session_cookie = http.header("Set-Cookie").substring(0, 37);
            DEBUG_PI_HOLE_PRINT("[PI] session: %s...", _php_session_cookie.c_str());
            while (payload_client->available()) {
              String line = payload_client->readStringUntil('\n');
              //<div id="token" hidden>dzRSDSIG9GCwSTehSoM7pLW2+vcTmHWy6nsql69+Bac=</div>
              if (line.startsWith("<div id=\"token\"")) {
                // replace = with %3D
                _token = line.substring(23, 66) + "%3D";
                DEBUG_PI_HOLE_PRINT("[PI] token: %s\n", _token.c_str());
                http.end();
                return true;
              }
            }
            DEBUG_PI_HOLE_PRINT("[PI] no token\n");
        } else {
          DEBUG_PI_HOLE_PRINT("[PI] no PHPSESSID in %s\n", http.header("Set-Cookie").c_str());
        }
      } else {
        DEBUG_PI_HOLE_PRINT("[PI] no cookies\n");
      }
      http.end();
    }
    return false;
  }

  const String _index_url;
  const String _group_url;
  const String _persistent_key_cookie;
  const char* GET_BLACKLISTS = "action=get_domains&showtype=black&token="; 
  String _token;
  String _php_session_cookie;
};

PiHoleCtrl* pi_ctrl;

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);

  pi_ctrl = new PiHoleCtrl(PI_HOLE_HOST, PERSISTANT_KEY);

}


void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;
    pi_ctrl->get_blacklist_group(client);
    
  }

  delay(10000);
}
   