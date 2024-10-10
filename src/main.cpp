#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <qrcode.h>

// Pin definitions
#define TFT_CS D7   // Chip Select -> GPIO13
#define TFT_DC D4   // Data/Command -> GPIO2
#define TFT_RST D3  // Reset -> GPIO0
#define TFT_SCLK D1 // SCK -> GPIO5
#define TFT_MOSI D2 // MOSI -> GPIO4

const char *ssid = "REOM-Kb";
const char *password = "zaonpfreom1";
HTTPClient http;

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

String utf8rus(String source)
{
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
return target;
}



void setupWiFi()
{
  delay(10);
  Serial.begin(115200); // Initialize serial communication
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void drawQRCode(QRCode *qrcode, int x, int y, int size)
{
  for (int i = 0; i < qrcode->size; i++)
  {
    for (int j = 0; j < qrcode->size; j++)
    {
      if (qrcode_getModule(qrcode, i, j))
      {
        tft.fillRect(x + j * size, y + i * size, size, size, ST77XX_BLACK);
      }
      else
      {
        tft.fillRect(x + j * size, y + i * size, size, size, ST77XX_WHITE);
      }
    }
  }
}

void downloadAndDisplayImage(String imageUrl)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    // Determine protocol
    bool isHttps = imageUrl.startsWith("https://");

    // Choose appropriate client
    WiFiClient *client;
    if (isHttps)
    {
      WiFiClientSecure *secureClient = new WiFiClientSecure;
      secureClient->setInsecure(); // Use only for testing
      client = secureClient;
    }
    else
    {
      client = new WiFiClient;
    }

    // Initialize HTTP client
    http.setTimeout(15000);                                 // Optional: Increase timeout
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Optional
    http.begin(*client, imageUrl);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
      // Get the response stream
      WiFiClient *stream = http.getStreamPtr();

      // Read the image dimensions from the first 4 bytes
      uint8_t header[4];
      int bytesRead = stream->readBytes(header, 4);
      if (bytesRead != 4)
      {
        Serial.println("Failed to read image dimensions");
        return;
      }

      uint16_t width = (header[0] << 8) | header[1];
      uint16_t height = (header[2] << 8) | header[3];

      Serial.printf("Image dimensions: %d x %d\n", width, height);
      const int offset = 5;
      // Now read the pixel data and draw the image
      for (int y = 0; y < height; y++)
      {
        for (int x = offset; x < width + offset; x++)
        {
          // Read 2 bytes for each pixel
          uint8_t pixelData[2];
          int bytesRead = stream->readBytes(pixelData, 2);
          if (bytesRead != 2)
          {
            Serial.println("Failed to read pixel data");
            return;
          }

          uint16_t color = (pixelData[0] << 8) | pixelData[1];

          tft.drawPixel(x, y, color);
        }
      }
    }
    else
    {
      Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    delete client; // Clean up
  }
  else
  {
    Serial.println("WiFi not connected");
  }
}
void setup()
{
  tft.cp437(true);
  setupWiFi();
  // Initialize display
  tft.init(240, 320);
  tft.setRotation(1);
  tft.setSPISpeed(80000000);
  tft.fillScreen(ST77XX_WHITE);
  //  Create QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, "https://192.168.0.16:10000/list/1/1");
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); // White text with black background (optional)
  tft.setTextSize(2);                           // Set text size
  tft.setCursor(25, 180);                       // Set position below images (adjust as needed)

  tft.println(utf8rus("Название: Цветочек"));
  tft.setCursor(25, 200);                       // Set position below images (adjust as needed)
  tft.println(utf8rus("Количество: 10"));
  // Draw QR code
  downloadAndDisplayImage("https://192.168.0.16:3002/node/get/14");
  drawQRCode(&qrcode, 170, 2, 5);
  
}

void loop()
{
  // Your main loop code
}
