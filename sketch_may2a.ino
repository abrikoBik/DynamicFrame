#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "FS.h"
#include "LittleFS.h"


TFT_eSPI tft = TFT_eSPI(320, 240);

const char* SSID = "DynamicFrame";
const char* password = "11111111";

AsyncWebServer server(80);
File fsUploadFile;

uint8_t uploaded_count = 0;
String uploaded_images[50];

int photo_delay = 5000;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void syncPhotos() {
  File root = LittleFS.open("/images");

  File photo = root.openNextFile();
  while(photo) {
    uploaded_images[uploaded_count++] = photo.path();
    photo = root.openNextFile();
  }

  root.close();
}

void setup() {
  Serial.begin(115200);

  //--------------------------------------//
  //             Access Point             //
  //--------------------------------------//
  Serial.println(SSID);
  WiFi.softAP(SSID, password, 1, false, 4);
  Serial.println(WiFi.softAPIP());

  if (!LittleFS.begin()) {
    Serial.println("LittleFS failed");
    while (1);
  }

  if(!LittleFS.exists("/images")) {
    LittleFS.mkdir("/images");
    Serial.println("Created dir /images!");
  } else {
    syncPhotos();
    Serial.println("Photos synced!");
  }

  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);

  TJpgDec.setCallback(tft_output);

  setupRoutes();
  server.begin();
}

void setupRoutes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/site/index.html", String(), false);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/site/style.css", "text/css", false);
  });

  server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/site/main.js", "application/javascript", false);
  });

  server.on("/cool.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/cool.jpg", "image/jpeg", false);
  });

  server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest *request){
    listDir(LittleFS, "/", 1);
    
    String res_arr = "{\"files\":[";
    for(int i = 0; i < uploaded_count; i++) {
      res_arr += "\"" + uploaded_images[i] + "\"";
      if (i < uploaded_count - 1) {
        res_arr += ",";
      }
    }
    res_arr += "],\"uploaded_count\":" + String(uploaded_count) + "}";
    
    request->send(200, "application/json", res_arr);
  });

  server.on("/api/images", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("name")) {
      String image_path = request->getParam("name")->value();
      request->send(LittleFS, image_path, "image/jpeg", false);
    } else {
      request->send(400, "text/plain", "NO PARAM");
    }
  });

  server.on("/api/files", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    if(request->hasParam("index")) {
      int idx = request->getParam("index")->value().toInt();
      
      if(idx >= 0 && idx < uploaded_count) {
        String fileToDelete = uploaded_images[idx];
        Serial.printf("Deleting %s...\n", fileToDelete.c_str());
        
        LittleFS.remove(fileToDelete);
        
        for(int i = idx; i < uploaded_count - 1; i++) {
          uploaded_images[i] = uploaded_images[i+1];
        }
        uploaded_count--;
        
        request->send(200, "text/plain", "OK");
      } else {
        request->send(400, "text/plain", "Invalid index");
      }
    } else {
      request->send(400, "text/plain", "Missing index parameter");
    }
  });

  server.on("/api/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "OK");
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    handleFileUpload(request, filename, index, data, len, final);
  });

  server.on("/api/space_used", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("%d/%d Bytes/Bytes\n", LittleFS.usedBytes(), LittleFS.totalBytes());
    request->send(200, "text/plain", String(LittleFS.usedBytes()));
  });

  server.on("/api/images/delay", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(!request->hasParam("delay", true)) {
      request->send(400, "text/plain", "NO ARG");
      return;
    }

    photo_delay = request->getParam("delay", true)->value().toInt();

    request->send(200, "text/plain", "OK");
  });

  server.on("/api/cur_delay", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(photo_delay));
  });
}

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    if (!filename.startsWith("/images/")) filename = "/images/" + filename;
    
    Serial.printf("UploadStart: %s\n", filename.c_str());
    
    if (uploaded_count >= 50) {
      Serial.println("Max files limit (50) reached!");
      return;
    }

    fsUploadFile = LittleFS.open(filename, FILE_WRITE);
    if (!fsUploadFile) {
      Serial.println("Failed to open file for writing");
      return;
    }

    bool isNew = true;
    for(int i = 0; i < uploaded_count; i++) {
      if(uploaded_images[i] == filename) {
        isNew = false;
        break;
      }
    }
    
    if(isNew) {
      uploaded_images[uploaded_count++] = filename;
    }
  }

  if (len && fsUploadFile) {
    fsUploadFile.write(data, len);
  }

  if (final) {
    if(fsUploadFile) fsUploadFile.close();
    Serial.printf("UploadEnd: %s, %u bytes\n", filename.c_str(), index + len);
  }
}

void showJpgFullscreen(const char* filename, fs::FS& fs) {
  uint16_t w, h;
  TJpgDec.getFsJpgSize(&w, &h, filename, fs);
  TJpgDec.setJpgScale(1);

  int16_t x = (tft.width()  - w) / 2;
  int16_t y = (tft.height() - h) / 2;

  TJpgDec.drawFsJpg(x, y, filename, fs);
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("%s\r\n", dirname);

  File root = fs.open(dirname);

  File file = root.openNextFile();
  while (file) {
      if (file.isDirectory()) {
          Serial.printf("\t%s\/\n", file.name());
          if (levels) {
              listDir(fs, file.path(), levels - 1);
              Serial.println("------------------------------");
          }
      } else {
          Serial.printf("\t%s - %d bytes\n",file.name(),file.size());
      }
      file = root.openNextFile();
  }
}

void loop() {
  static int lastShow = 0;
  static int curImage = 0;
  
  if(millis() - lastShow >= photo_delay) {
    lastShow = millis();
    
    if(uploaded_count == 0) {
      showJpgFullscreen("/cool.jpg", LittleFS);
    } else {
      if(curImage >= uploaded_count) {
        curImage = 0;
      }
      showJpgFullscreen(uploaded_images[curImage].c_str(), LittleFS);
      curImage++;
    }
  }
}