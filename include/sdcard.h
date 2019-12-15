//***************************************************
// SDCard SPI related parameter file
//***************************************************

#define Select    LOW       //  Low CS means that SPI device Selected
#define DeSelect  HIGH      //  High CS means that SPI device Deselected

#define SdFile File
#define seekSet seek

void listDir    (fs::FS &fs, const char * dirname, uint8_t levels);
void createDir  (fs::FS &fs, const char * path);
void removeDir  (fs::FS &fs, const char * path);
void readFile   (fs::FS &fs, const char * path);
void writeFile  (fs::FS &fs, const char * path, const char * message);
void appendFile (fs::FS &fs, const char * path, const char * message);
void renameFile (fs::FS &fs, const char * path1, const char * path2);
void deleteFile (fs::FS &fs, const char * path);
void testFileIO (fs::FS &fs, const char * path);

// end of sdcard.h