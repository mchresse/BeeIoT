//*******************************************************************
// CONFIG.INI - BeeIoT - Main Configuration file
//*******************************************************************
/* Map of used ESP32-DevKit-C PIN Configuration:
* For Default board settings see also 
*    C:\Users\MCHRESSE\.platformio\packages\framework-arduinoespressif32\variants\esp32
*================================================================
|Pin| Ref. |  IO# |  DevKitC |       | Protocol |  Components    |
|---|------|------|----------|-------|----------|----------------|
|  1| +3.3V|      |          |       |    +3.3V | -              |
|  2|    EN|      |   SW2    |       |          | ePaper-Key2    |
|  3|*  SVP|GPIO36|  Sens-VP | ADC1-0|        - | -              |
|  4|*  SVN|GPIO39|  Sens-VN | ADC1-3|        - | -              |
|  5|* IO34|GPIO34|          | ADC1-6|          | ePDK3/LoRa DIO2|
|  6|* IO35|GPIO35|          | ADC1-7|          | ePaper-Key4    |
|  7|  IO32|GPIO32|   XTAL32 | ADC1-4|OneWire-SD| DS18B20(3x)    |
|  8|  IO33|GPIO33|   XTAL32 | ADC1-5|          | LoRa DIO0      |
|  9|  IO25|GPIO25|     DAC1 | ADC2-8|  Wire-DT | HX711-DT       |
| 10|  IO26|GPIO26|     DAC2 | ADC2-9|  Wire-Clk| HX711-SCK      |
| 11|  IO27|GPIO27|          | ADC2-7| ADS-Alert| ADS1115/BMS    |
| 12|  IO14|GPIO14| HSPI-CLK | ADC2-6|Status-LED| LoRa RST       |
| 13|  IO12|GPIO12| HSPI-MISO| ADC2-5| SPI1-MISO| LoRa CS\       |
| 14|   GND|      |          |       |       GND| -              |
| 15|  IO13|GPIO13| HSPI-MOSI| ADC2-4| SPI1-MOSI| LoRa DIO1      |
| 16|   SD2|GPIO09|FLASH-D2  | U1-RXD|        - | -              |
| 17|   SD3|GPIO10|FLASH-D3  | U1-TXD|        - | -              |
| 18|   CMD|GPIO11|FLASH-CMD | U1-RTS|        - | -              |
| 19|   +5V|      |          |       |      +5V | -              |
| 20|   CLK|GPIO06|FLASH-CLK | U1-CTS|        - | -              |
| 21|   SD0|GPIO07|FLASH-D0  | U2-RTS|        - | -              |
| 22|   SD1|GPIO08|FLASH-D1  | U2-CTS|        - | -              |
| 23|  IO15|GPIO15| HSPI-SS  | ADC2-3|MonLED-red| Red LED        |
| 24|  IO02|GPIO02| HSPI-4WP | ADC2-2|  SPI-CS  | SDcard CS\     |
| 25|  IO00|GPIO00|   SW1    | ADC2-1|          | ePaper-Key1    |
| 26|  IO04|GPIO04| HSPI-4HD | ADC2-0|      CLK | ePaper BUSY    |
| 27|  IO16|GPIO16|UART2-RXD |       |      DT  | ePaper RST     |
| 28|  IO17|GPIO17|UART2-TXD |       |          | ePaper D/C     |
| 29|  IO05|GPIO05| VSPI-SS  |       | SPI0-CS0 | ePD CS\        |
| 30|  IO18|GPIO18| VSPI-CLK |       | SPI0-Clk | ePD/SD/LoRa Clk|
| 31|  IO19|GPIO19| VSPI-MISO| U0-CTS| SPI0-MISO| SD/LoRa MISO   |
| 32|   GND|      |          |       |      GND | -              |
| 33|  IO21|GPIO21| VSPI-4HD |       |  I2C-SDA | ADS1115/BMS    |
| 34|  RXD0|GPIO03|UART0-RX  | U0-RXD| -> USB   | -              |
| 35|  TXD0|GPIO01|UART0-TX  | U0-TXD| -> USB   | -              |
| 36|  IO22|GPIO22| VSPI-4WP | U0-RTS|  I2C-SCL | ADS1115/BMS    |
| 37|  IO23|GPIO23| VSPI-MOSI|       | SPI0-MOSI|ePD/SD/LoRa MOSI|
| 38|   GND|      |          |       |       GND| -              |
*/
//*******************************************************************

//*******************************************************************
// Device configuration Matrix field (default: all active)
// 0 = active, uncomment = inactive
//*******************************************************************
#define HX711_CONFIG  0
#define ADS_CONFIG    0
#define ONEWIRE_CONFIG 0
#define WIFI_CONFIG   0
#define NTP_CONFIG    0
#define SD_CONFIG     0
#define EPD_CONFIG    0
#define LORA_CONFIG	  0

// Pin mapping when using SDCARD in SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.

#define HSPI_MISO   12    // PIN_NUM_MISO
#define HSPI_MOSI   13    // PIN_NUM_MOSI
#define HSPI_SCK    14    // PIN_NUM_CLK
#define HSPI_CS     15    // PIN_NUM_CS

// ESP32 default SPI port pins:
#define VSPI_MISO   MISO  // PIN_NUM_MISO    = 19
#define VSPI_MOSI   MOSI  // PIN_NUM_MOSI    = 23
#define VSPI_SCK    SCK   // PIN_NUM_CLK     = 18
#define VSPI_CS     SS    // PIN_NUM_CS      = 5

#define SD_MISO     MISO  // SPI MISO -> VSPI = 19
#define SD_MOSI     MOSI  // SPI MOSI -> VSPI = 23
#define SD_SCK      SCK   // SPI SCLK -> VSPI = 18
#define SD_CS       2     // SD card CS line - arbitrary selection !

#define LED_RED     15    // GPIO number of red LED
// reused by BEE_RST: Green LED not used anymore
//#define LED_GREEN   14    // GPIO number of green LED

// HX711 ADC GPIO Port to Espressif 32 board
#define HX711_DT    25    // serial dataline
#define HX711_SCK   26    // Serial clock line

// ADS1115 + RTC DS3231 - I2C Port
#define ADS_ALERT   27    // arbitrary selection of ALERT line
#define ADS_SDA     SDA    // def: SDA=21
#define ADS_SCL     SCl    // def. SCL=22

// OneWire Data Port:
#define ONE_WIRE_BUS 32

// WavePaper ePaper port
// mapping suggestion for ESP32 DevKit or LOLIN32, see .../variants/.../pins_arduino.h for your board
// Default: BUSY -> 4,       RST -> 16, 	  DC  -> 17, 	CS -> SS(5), 
// 			CLK  -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V
#define EPD_MISO VSPI_MISO 	// SPI MISO -> VSPI
#define EPD_MOSI VSPI_MOSI 	// SPI MOSI -> VSPI
#define EPD_SCK  VSPI_SCK  	// SPI SCLK -> VSPI
#define EPD_CS       5     	// SPI SS   -> VSPI
#define EPD_DC      17     	// arbitrary selection of DC   > def: 17
#define EPD_RST     16     	// arbitrary selection of RST  > def: 16
#define EPD_BUSY     4     	// arbitrary selection of BUSY > def:  4  -> if 35 -> RD only GPIO !
#define EPD_KEY1     0     	// via 40-pin RPi slot at ePaper Pin29 (P5)
#define EPD_KEY2    EN     	// via 40-pin RPi slot at ePaper Pin31 (P6)
#define EPD_KEY3    34     	// via 40-pin RPi slot at ePaper Pin33 (P13)
#define EPD_KEY4    35     	// via 40-pin RPi slot at ePaper Pin35 (P19)

// LoRa-Bee Board
#define BEE_MISO VSPI_MISO	// SPI MISO -> VSPI
#define BEE_MOSI VSPI_MOSI	// SPI MOSI -> VSPI
#define BEE_SCK  VSPI_SCK	// SPI SCLK -> VSPI
#define BEE_CS   	12		// NSS
#define BEE_RST		14  	//
#define BEE_DIO0	33		// Main Lora_Interrupt line
#define BEE_DIO1	13		// Bee-Event
#define BEE_DIO2	34		// unused by BEE_Lora;  EPD K3 -> but is a RD only GPIO !

#define LOOPTIME    360		// [sec] Loop wait time: 60 for 1 Minute
#define SLEEPTIME   3E7		// Mikrosekunden hier 3s

// Definitions of LogLevel masks instead of verbose mode
#define LOGBH		1		// 1:   Behive Setup & Loop program flow control
#define LOGOW		2		// 2:   1-wire discovery and value read
#define LOGHX		4		// 4:   HX711 init & get values
#define LOGEPD		8		// 8:   ePaper init & control
#define LOGLAN		16		// 16:  Wifi init & LAN Import/export in all formats and protocols
#define LOGSD		32		// 32:  SD Card & local data handling
#define LOGADS		64  	// 64:  ADS BMS monitoring routines /w ADS1115S
#define LOGSPI		128		// 128: SPI Init
#define LOGLORA		256		// 256: LoRa Init

#define	BHLOG(m)	if(lflags & m)	// macro for Log evaluation

#define NETWORKSTATE_LOGGED       1
#define NETWORKSTATE_NOTLOGGED    0
#define SDLOGPATH	"/logdata.txt"				// path of logfile for sensor data set bursts

#define NODESTATE_RESET                    3    // S3 = connection + subscription established after user reset
#define NODESTATE_PING                     2    // S2 = connection still open, successful ping
#define NODESTATE_OK                       1    // S1 = connection + subscription established
#define NODESTATE_NOTSPECIFIEDERROR        0    // E0 = unspecified error
#define NODESTATE_NOTCPCONNECTION         -1    // E1 = connection not established because of no tcp connection to test broker
#define NODESTATE_NOMQTTCONNECTION        -2    // E2 = connection not established because broker rejected MQTT connection attempt
#define NODESTATE_NOMQTTSUBSCRIPTION      -3    // E3 = broker rejected subscription attempt
#define NODESTATE_NOPING                  -4    // E4 = ping was not successful, connection lost

// Battery thresholds for LiPo 3.7V battery type
#ifndef BATTERY_MAX_LEVEL
    #define BATTERY_MAX_LEVEL        4200 // mV 
    #define BATTERY_MIN_LEVEL        3300 // mV
    #define BATTERY_SHUTDOWN_LEVEL   3200 // mV
#endif


//*******************************************************************
// Global data declarations
//*******************************************************************

#define DROWNOTELEN	129
#define LENTMSTAMP	20
typedef struct {				// data elements of one log line entry
    int     index;
    char  	timeStamp[LENTMSTAMP]; // time stamp of sensor row  e.g. 'YYYY\MM\DD HH:MM:SS'
	float	HiveWeight;			// weight in [kg]
	float	TempHive;			// internal temp. of bee hive
	float	TempExtern;			// external temperature
	float	TempIntern;			// internal temp. of weight cell (for compensation)
	float	TempRTC;			// internal temp. of RTC module
	int16_t ESP3V;				// ESP32 DevKit 3300 mV voltage level
	int16_t Board5V;			// Board 5000 mV Power line voltage level
	int16_t BattCharge;			// Battery Charge voltage input >0 ... 5000mV 
	int16_t BattLoad;			// Battery voltage level (typ. 3700 mV)
	int16_t BattLevel;			// Battery Level in % (3200mV (0%) - 4150mV (100%))
	char	 comment[DROWNOTELEN];
} datrow;

#define datasetsize	6			// max. # of dynamic dataset buffer: each for "looptime" seconds
#define LENFDATE 	21
#define LENDATE		11
#define LENTIME		9
#define LENIPADDR	12
typedef struct {
	// save timestamp of last datarow entry:
    char	formattedDate[LENFDATE]; // Variable to save date and time; 2019-10-28T08:09:47Z
    char	date[LENDATE];  // stamp of the day: 2019\10\28
    char	time[LENTIME]; // Timestamp: 08:10:15
    char	ipaddr[LENIPADDR];	// IPv4 Address xx:xx:xx:xx
    int     loopid;             // sensor data read sample ID
	int		laps;				// # of hangovers till datasetsize reached by loopid++
    uint64_t  BoardID;          // unique Identifier of MUC board (6 Byte effective)
	datrow	dlog[datasetsize];	// all sensor logs till upload to server
} dataset;


//*******************************************************************
// Global Helper functions declaration
//*******************************************************************
int setup_hx711Scale(void);
int setup_owbus		(void);
int setup_wifi		(void);
int setup_ntp		(void);
int setup_i2c_ADS	(void);

// in hx711Scale.cpp
float   HX711_read  (int mode);

// in wifi.cpp
void    wifi_scan   (void);
int     wifi_connect(const char * ssid, const char * pwd, const char* hostname);
int 	wifi_disconnect();
int		WiFi_AccessPointStart(char* AccessPointNetworkSSID);
void 	WiFi_SetBothModesNetworkStationAndAccessPoint();
String 	WiFi_GetOwnIPAddressInRouterNetwork();

void 	Webserver_Start();
String 	Webserver_GetRequestGETParameter();
void 	Webserver_SendHTMLPage(String HTMLPage);
String 	EncodeFormHTMLFromValues(String TitleOfForm, int CountOfConfigValues);
int 	DecodeGETParameterAndSetConfigValues(String GETParameter);
int 	ConnectToATT();
void 	SendSensorDateToATT(String SensorName, byte SensorDate);

// in rtc.cpp
int 	setup_rtc	(void);
int 	ntp2rtc		(void);
int 	getRTCtime	(void);
void	rtc_test	(void);

// in getntp.cpp
int    	getTimeStamp(void);
void	printLocalTime(void);

// in owbus.cpp
void 	GetOWsensor	(int sample);

// in i2c_ads.cpp
void    i2c_scan    (void);
int16_t ads_read	(int channel);

// in main.cpp
void	SDlogdata   (void);
void	mydelay		(int32_t tval);

// in multiSPI.cpp
int setup_spi_VSPI	(void);

//in sdcard.cp
int setup_sd	(void);

// epd.cpp functions
int  setup_epd		(void);
void drawBitmapFromSD(const char *filename, int16_t x, int16_t y, bool with_color = true);
void drawBitmaps_200x200();
void drawBitmaps_other(void);
void showdata		(int sampleID);

// BeeLoRa.cpp functions
int  setup_LoRa		(void);
int sendMessage	(byte cmd, String outgoing);
void onReceive		(int packetSize);
void hexdump		(unsigned char * msg, int len);
// end of BeeIoT.h