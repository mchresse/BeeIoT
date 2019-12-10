//*******************************************************************
// ePaper Libraries
//*******************************************************************
#include <Arduino.h>
#include <SPI.h>
// for defaut ESP32 SPI pin definitions see e.g.:
// C:\Users\xxx\Documents\Arduino\hardware\espressif\esp32\variants\lolin32\pins_arduino.h

// Libraries for SD card
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "sdcard.h"
#include "wificfg.h"

// Libs for WaveShare ePaper 2.7 inch r/w/b Pinning GxGDEW027C44
#include <GxEPD.h>
#include <GxGDEW027C44/GxGDEW027C44.h>  // 2.7" b/w/r
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

#define HAS_RED_COLOR     // as defined in GxGDEW027C44.h: GxEPD_WIDTH, GxEPD_HEIGHT

// #include "BitmapGraphics.h"
// #include "BitmapExamples.h"
#include "BitmapWaveShare.h"

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

#include <beeiot.h>

//************************************
// Global data object declarations
//**********************************
extern GxEPD_Class display;   // ePaper instance from MultiSPI Module
extern int isepd;             // =0 ePaper found
extern dataset bhdb;          // central BeeIo DB
extern uint16_t	lflags;      // BeeIoT log flag field


uint16_t read16(SdFile& f);
uint32_t read32(SdFile& f);

//*******************************************************************
// ePaper Setup Routine
//*******************************************************************
int setup_epd() {   // My EPD Constructor
#ifdef EPD_CONFIG
  BHLOG(LOGEPD) Serial.println("  Setup: EPD ePaper");

  if(isepd==0){ // set some presets for a std. text field
    BHLOG(LOGEPD) Serial.println("  EPD: print Welcome Display");
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(0, 0);
    display.setRotation(45);    // print in horizontal format

    display.setFont(&FreeMonoBold24pt7b);
    display.setCursor(0, 2*24);
    display.println("BeeIoT.v2");
    display.setFont(&FreeMonoBold12pt7b);
    display.println("     by R.Esser");
    display.setFont(&FreeMonoBold9pt7b);
    display.printf ("    BoardID: %08X\n", (uint32_t)bhdb.BoardID);    

    display.update();    //refresh display by buffer content

    BHLOG(LOGEPD) delay(5000);
    BHLOG(LOGEPD) Serial.println("  EPD: Draw BitmapWaveshare");
    BHLOG(LOGEPD) display.setRotation(2);
    BHLOG(LOGEPD) display.drawExampleBitmap(BitmapWaveshare, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    BHLOG(LOGEPD) display.update();
  }  
#endif

  // Preset EPD-Keys 1-4: active 0 => connects to GND (needs a pullup)
  // EPD_KEY1 => GPIO00 == Boot Button
  // EPD_KEY2 => EN     == Reset Button 
  pinMode(EPD_KEY3, INPUT_PULLUP);  // define as active 0: Key3 
  pinMode(EPD_KEY4, INPUT_PULLUP);  // define as active 0: Key4
  // Key3+4 are free of use

  return 0;
}

void drawBitmaps_200x200(){
  int16_t x = (display.width() - 200) / 2;
  int16_t y = (display.height() - 200) / 2;
  drawBitmapFromSD("logo200x200.bmp", x, y);
  delay(2000);
}

void drawBitmaps_other(){
  int16_t w2 = display.width() / 2;
  int16_t h2 = display.height() / 2;
  drawBitmapFromSD("parrot.bmp", w2 - 64, h2 - 80);
  delay(2000);
  drawBitmapFromSD("betty_1.bmp", w2 - 100, h2 - 160);
  delay(2000);
 }


static const uint16_t input_buffer_pixels = 20;  // may affect performance
static const uint16_t max_palette_pixels  = 256; // for depth <= 8

uint8_t input_buffer[3 * input_buffer_pixels];        // up to depth 24
uint8_t mono_palette_buffer[max_palette_pixels / 8];  // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w

void drawBitmapFrom_SD_ToBuffer(const char *filename, int16_t x, int16_t y, bool with_color){
  SdFile file;
  bool valid = false; // valid format to be handled
  bool flip = true; // bitmap is stored bottom-to-top
  uint32_t startTime = millis();


  if ((x >= display.width()) || (y >= display.height())) {
    BHLOG(LOGEPD) Serial.printf("  EPD: BitMap position out of range (%i, %i)\n", x, y);
    return;
  }
  BHLOG(LOGEPD) Serial.printf("  EPD: Loading BitMap from SD: %s at (%i, %i)\n", filename, x, y);

  file = SD.open(String("/") + filename, FILE_READ);
  if (!file){
    BHLOG(LOGEPD) Serial.printf("  EPD: File <%s> not found\n", filename);
    return;
  }
  // Parse BMP header
  if (read16(file) == 0x4D42) // BMP signature
  {
    uint32_t fileSize = read32(file);
    uint32_t imageOffset = read32(file); // Start of image data
    uint32_t headerSize = read32(file);
    uint32_t width  = read32(file);
    uint32_t height = read32(file);
    uint16_t planes = read16(file);
    uint16_t depth = read16(file); // bits per pixel
    uint32_t format = read32(file);
    if ((planes == 1) && ((format == 0) || (format == 3))){ // uncompressed is handled, 565 also
      Serial.print("File size: "); Serial.println(fileSize);
      Serial.print("Image Offset: "); Serial.println(imageOffset);
      Serial.print("Header size: "); Serial.println(headerSize);
      Serial.print("Bit Depth: "); Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8) rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0){
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.width())  w = display.width()  - x;
      if ((y + h - 1) >= display.height()) h = display.height() - y;
      valid = true;
      uint8_t bitmask = 0xFF;
      uint8_t bitshift = 8 - depth;
      uint16_t red, green, blue;
      bool whitish=0;
      bool colored=0;
      if (depth == 1) with_color = false;
      if (depth <= 8){
        if (depth < 8) bitmask >>= depth;
        file.seekSet(54); //palette is always @ 54
        for (uint16_t pn = 0; pn < (1 << depth); pn++){
          blue  = file.read();
          green = file.read();
          red   = file.read();
          file.read();
          whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
          colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
          if (0 == pn % 8) mono_palette_buffer[pn / 8] = 0;
          mono_palette_buffer[pn / 8] |= whitish << pn % 8;
          if (0 == pn % 8) color_palette_buffer[pn / 8] = 0;
          color_palette_buffer[pn / 8] |= colored << pn % 8;
        }
      }
      display.fillScreen(GxEPD_WHITE);
      uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
      for (uint16_t row = 0; row < h; row++, rowPosition += rowSize){ // for each line
        uint32_t in_remain = rowSize;
        uint32_t in_idx = 0;
        uint32_t in_bytes = 0;
        uint8_t in_byte = 0; // for depth <= 8
        uint8_t in_bits = 0; // for depth <= 8
        uint16_t color = GxEPD_WHITE;
        file.seekSet(rowPosition);
        for (uint16_t col = 0; col < w; col++){ // for each pixel
          // Time to read more pixel data?
          if (in_idx >= in_bytes){ // ok, exact match for 24bit also (size IS multiple of 3)
            in_bytes = file.read(input_buffer, in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain);
            in_remain -= in_bytes;
            in_idx = 0;
          }
          switch (depth){
            case 24:
              blue = input_buffer[in_idx++];
              green = input_buffer[in_idx++];
              red = input_buffer[in_idx++];
              whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
              colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
              break;
            case 16:{
                uint8_t lsb = input_buffer[in_idx++];
                uint8_t msb = input_buffer[in_idx++];
                if (format == 0){ // 555
                  blue  = (lsb & 0x1F) << 3;
                  green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                  red   = (msb & 0x7C) << 1;
                } else { // 565
                  blue  = (lsb & 0x1F) << 3;
                  green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                  red   = (msb & 0xF8);
                }
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80);
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
              }
              break;
            case 1:
            case 4:
            case 8: {
                if (0 == in_bits){
                  in_byte = input_buffer[in_idx++];
                  in_bits = 8;
                }
                uint16_t pn = (in_byte >> bitshift) & bitmask;
                whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                in_byte <<= depth;
                in_bits -= depth;
              }
              break;
          }
          if (whitish){
            color = GxEPD_WHITE;
          } else if (colored && with_color){
            color = GxEPD_RED;
          } else {
            color = GxEPD_BLACK;
          }
          uint16_t yrow = y + (flip ? h - row - 1 : row);
          display.drawPixel(x + col, yrow, color);
        } // end pixel
      } // end line
      BHLOG(LOGEPD) Serial.print("  EPD: Image loaded in "); Serial.print(millis() - startTime); Serial.println(" ms");
    }
  }
  file.close();
  if (!valid){
      BHLOG(LOGEPD) Serial.println("  EPD: Bitmap format not Supported");
  }
} // end of drawBitmapFrom_SD_ToBuffer()


#if defined(__AVR) //|| true
struct Parameters {
  const char* filename;
  int16_t x;
  int16_t y;
  bool    with_color;
};

void drawBitmapFrom_SD_ToBuffer_Callback(const void* params) {
  const Parameters* p = reinterpret_cast<const Parameters*>(params);
  drawBitmapFrom_SD_ToBuffer(p->filename, p->x, p->y, p->with_color);
}

void drawBitmapFromSD(const char *filename, int16_t x, int16_t y, bool with_color) {
  Parameters parameters{filename, x, y, with_color};
  display.drawPaged(drawBitmapFrom_SD_ToBuffer_Callback, &parameters);
}

#else

void drawBitmapFromSD(const char *filename, int16_t x, int16_t y, bool with_color){
  drawBitmapFrom_SD_ToBuffer(filename, x, y, with_color);
  display.update();
}
#endif

uint16_t read16(SdFile& f){
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(SdFile& f){
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void showFont(const char name[], const GFXfont* f){
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
  display.println(name);
  display.println(" !\"#$%&'()*+,-./");
  display.println("0123456789:;<=>?");
  display.println("@ABCDEFGHIJKLMNO");
  display.println("PQRSTUVWXYZ[\\]^_");
#if defined(HAS_RED_COLOR)
  display.setTextColor(GxEPD_RED);
#endif
  display.println("`abcdefghijklmno");
  display.println("pqrstuvwxyz{|}~ ");
  display.update();
}

void showFontCallback()
{
  const char* name = "FreeMonoBold9pt7b";
  const GFXfont* f = &FreeMonoBold9pt7b;
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
  display.println(name);
  display.println(" !\"#$%&'()*+,-./");
  display.println("0123456789:;<=>?");
  display.println("@ABCDEFGHIJKLMNO");
  display.println("PQRSTUVWXYZ[\\]^_");
#if defined(HAS_RED_COLOR)
  display.setTextColor(GxEPD_RED);
#endif
  display.println("`abcdefghijklmno");
  display.println("pqrstuvwxyz{|}~ ");
}

void showPartialUpdate(float data){
  String dataString = String(data,1);
  // const char* name = "FreeSansBold24pt7b";
  const GFXfont* f = &FreeSansBold24pt7b;
  
  uint16_t box_x = 60;
  uint16_t box_y = 60;
  uint16_t box_w = 90;
  uint16_t box_h = 100;
  uint16_t cursor_y = box_y + 16;

//  display.setRotation(45);
  display.setFont(f);
  display.setTextColor(GxEPD_BLACK);

  Serial.print("  Loop: Clear Screen and update partial");
  display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
  display.setCursor(box_x, cursor_y+38);
  display.print(dataString); 
  display.updateWindow(box_x, box_y, box_w, box_h, true);
}


void showdata(int sampleID){
  uint8_t rotation = display.getRotation();

  display.fillScreen(GxEPD_WHITE);

  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(0, 0);
  display.setRotation(45);    // print in horizontal format
  display.println();          // adjust cursor to lower left corner of char row

  display.setFont(&FreeMonoBold12pt7b);
  display.printf("BeeIoT.v2   #%i", (bhdb.laps*datasetsize) + sampleID);

  display.setFont(&FreeMonoBold9pt7b);
  display.println();  

  display.setTextColor(GxEPD_RED);
  display.printf("%s %s", bhdb.dayStamp.c_str(), bhdb.timeStamp.c_str());
  
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold12pt7b);
  display.println();  

  display.print(" Gewicht:  ");
  display.println(String(bhdb.dlog[sampleID].HiveWeight,3));

  display.print(" Temp.Beute: ");
  display.println(String(bhdb.dlog[sampleID].TempHive,1));

  display.print(" TempExtern: ");
  display.println(String(bhdb.dlog[sampleID].TempExtern,1));

  display.print(" TempIntern: ");
  display.println(String(bhdb.dlog[sampleID].TempIntern,1));

  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.print("Batt-V:");
  display.print(String((float)bhdb.dlog[sampleID].BattLoad/1000,2));
  display.print("V - Load:");
  display.print(String(bhdb.dlog[sampleID].BattLevel));
  display.println("%");

  display.setTextColor(GxEPD_RED);
  display.print(HOSTNAME);
  display.print("  (");
  display.print(bhdb.ipaddr.c_str());
  display.print(")");

  display.setTextColor(GxEPD_BLACK);
//  display.writeFastHLine(0, 16, 2, 0xFF); // does not work

  display.update();
  display.setRotation(rotation); // restore
} // end of ShowData()
