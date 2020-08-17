

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

ESP8266WiFiMulti WiFiMulti;
String mainURL ="http://192.168.1.110";

#define WIFI_SSID ""
#define WIFI_PASS ""
#define PERSISTANT_KEY "persistentlogin="

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

}

#define LINE_BUFFER_SIZE  1024
char line_buffer[LINE_BUFFER_SIZE+1];

int get_next_line(WiFiClient* stream_ptr) {
  int tail = 0;
  while(true){
    int c = stream_ptr->read();
    if (c <= 0) {
      return c;
    }

    line_buffer[tail++] = c;
    if (c == '\n') {
      line_buffer[tail] = 0;
      return tail;
    }

    if (tail == LINE_BUFFER_SIZE) {
      return -2;
    }
  }
}

void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...");
    if (http.begin(client, mainURL+"/admin/index.php")) {  // HTTP
    //if (http.begin(client, "192.168.1.103", 8000)) {  // HTTP
      http.addHeader("Cookie", PERSISTANT_KEY);

      const char * headers[] = {"Set-Cookie"};
      Serial.print("[HTTP] GET...");
      http.collectHeaders(headers, 1);
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d", httpCode);
        Serial.printf(" size: %d\n", http.getSize());

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          WiFiClient* payload_client = http.getStreamPtr();
          
          if (http.hasHeader("Set-Cookie")) {
            Serial.println(http.header("Set-Cookie"));
          }
          while (payload_client->available()) {
            
            // code = get_next_line(http.getStreamPtr());
            // if (code < 0) {
            //   break;
            // }
            // char *ptr = strstr(line_buffer, "Token");
            // if (ptr != NULL) /* Substring found */
            // {
            //   Serial.println(line_buffer);
            // }
            String line = payload_client->readStringUntil('\n');
            //<div id="token" hidden>dzRSDSIG9GCwSTehSoM7pLW2+vcTmHWy6nsql69+Bac=</div>
            if (line.startsWith("<div id=\"token\"")) {
              String token = line.substring(23, 67);
              Serial.println(token);
              break;
            }
          }
          Serial.println();
          
          
          //Serial.println(code);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect");
    }
  }

  delay(10000);
}
   