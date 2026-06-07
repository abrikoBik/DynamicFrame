#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "FS.h"
#include "LittleFS.h"

TFT_eSPI tft = TFT_eSPI(320, 240);

const char* SSID = "824";
const char* password = "251006428";

AsyncWebServer server(80);
File fsUploadFile;

char uploaded_count = 0;
String uploaded_images[13];

// Вывод в TFT
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi network
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (!LittleFS.begin()) {
    Serial.println("LittleFS failed");
    while (1);
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

  server.on("/api/listFS", HTTP_GET, [](AsyncWebServerRequest *request){
    listDir(LittleFS, "/", 1);
    // TODO: this request should send file array to client
    request->send(200, "application/json", "{\"status\":1}");
  });

  server.on("/api/upload", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "OK");
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    handleFileUpload(request, filename, index, data, len, final);
  });
}


void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    // Начало загрузки
    if (!filename.startsWith("/")) filename = "/" + filename;
    
    Serial.printf("UploadStart: %s\n", filename.c_str());
    
    // Удаляем файл если существует
    if(!LittleFS.exists(filename)) {
      // Открываем файл для записи
      uploaded_images[uploaded_count++] = filename;
      fsUploadFile = LittleFS.open(filename, FILE_WRITE);
    } else {
      Serial.printf("%s file already exists!\n", filename);
    }
    
    if (!fsUploadFile) {
      Serial.println("Failed to open file for writing");
      return;
    }
  }

  // Запись данных
  if (len) {
    fsUploadFile.write(data, len);
  }

  // Конец загрузки
  if (final) {
    fsUploadFile.close();
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
  if (!root) {
      Serial.println("- failed to open directory");
      return;
  }
  if (!root.isDirectory()) {
      Serial.println(" - not a directory");
      return;
  }

  File file = root.openNextFile();
  while (file) {
      if (file.isDirectory()) {
          Serial.printf("\t%s\/\n", file.name());
          if (levels) {
              listDir(fs, file.path(), levels - 1);
              Serial.printf("------------------------------");
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
  if(millis() - lastShow >= 5000) {
    lastShow = millis();
    if(!uploaded_count) {
      showJpgFullscreen("/cool.jpg", LittleFS);
    } else {
      if(curImage > uploaded_count) {
        curImage = 0;
      }
      showJpgFullscreen(uploaded_images[curImage++].c_str(), LittleFS);
    }
  }
}