#define ILI9341_DRIVER

#define TFT_RGB_ORDER TFT_RGB 

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5  // Chip select control pin
#define TFT_DC    16  // Data Command control pin
#define TFT_RST   17  // Reset pin (could connect to RST pin)

#define SPI_FREQUENCY  40000000