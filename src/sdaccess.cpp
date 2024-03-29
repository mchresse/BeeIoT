//*******************************************************************
// SDaccess.cpp
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// SD-Access - Support routines
//
//-------------------------------------------------------------------
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at:
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
// This Module contains code derived from
// - SD.h library example code:
//   https://github.com/espressif/arduino-esp32/blob/master/libraries/SD/examples/SD_Test/SD_Test.ino
//   provides basic helper functions for SD Card file system management
//    listDir()
//    createDir()
//    removeDir()
//    readFile()
//    writeFile()
//    appendFile()
//    renameFile()
//    deleteFile()
//    testFile()
//*******************************************************************

#include <Arduino.h>
#include <stdio.h>
#include <esp_log.h>
#include "sdkconfig.h"

//*******************************************************************
// SDCard SPI Library
//*******************************************************************

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "sdcard.h"

#include "beeiot.h"
#include "BeeIotWan.h"
extern uint16_t	lflags;      // BeeIoT log flag field

//*******************************************************************
// SD Access Routines
//*******************************************************************

void listDir(fs::FS &fs, const char * dirname, uint8_t levels, char * fileline ){
    BHLOG(LOGSD) Serial.printf("  SD: List directory: '%s'\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        BHLOG(LOGSD) Serial.printf("  SD: Failed to open directory '%s'\n", dirname);
        return;
    }
    if(!root.isDirectory()){
        BHLOG(LOGSD) Serial.printf("  SD: '%s' is not a directory\n", dirname);
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            BHLOG(LOGSD) Serial.print("    DIR : ");
            BHLOG(LOGSD) Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels-1, fileline);
            }
        } else {
			snprintf(fileline, (size_t) SDDIRLEN, "  FILE: %s, SIZE: %d", file.name(), file.size());
            BHLOG(LOGSD) Serial.print("      FILE: ");
            BHLOG(LOGSD) Serial.print(file.name());
            BHLOG(LOGSD) Serial.print("  SIZE: ");
            BHLOG(LOGSD) Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    BHLOG(LOGSD) Serial.printf("  SD: Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        BHLOG(LOGSD) Serial.println("  SD: Dir created");
    } else {
        BHLOG(LOGSD)Serial.println("  SD: mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    BHLOG(LOGSD) Serial.printf("  SD: Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        BHLOG(LOGSD) Serial.println("  SD: Dir removed");
    } else {
        Serial.println("  SD: rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    BHLOG(LOGSD) Serial.printf("  SD: Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        BHLOG(LOGSD) Serial.println("  SD: Failed to open file for reading");
        return;
    }

    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    BHLOG(LOGSD) Serial.printf("  SD: Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("  SD: Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        BHLOG(LOGSD) Serial.println("  SD: File written");
    } else {
        Serial.println("  SD: Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    BHLOG(LOGSD) Serial.printf("  SD: Appending to file: %s", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("  SD: ...Open file failed");
        return;
    }
    if(file.print(message)){
        BHLOG(LOGSD) Serial.println("...Done");
    } else {
        Serial.println("  SD: Print to file failed");
    }
    file.close();
}

void appendBinFile(fs::FS &fs, const char * path, const uint8_t* message, size_t length){
    BHLOG(LOGSD) Serial.printf("  SD: Appending to bin file: %s", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("  SD: ...Open file failed");
        return;
    }
    if(file.write(message, length)){
        BHLOG(LOGSD) Serial.println("...Done");
    } else {
        Serial.println("  SD: Print to file failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    BHLOG(LOGSD) Serial.printf("  SD: Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        BHLOG(LOGSD) Serial.println("  SD: File renamed");
    } else {
        Serial.println("  SD: Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    BHLOG(LOGSD) Serial.printf("  SD: Deleting file: %s\n", path);
    if(fs.remove(path)){
        BHLOG(LOGSD) Serial.println("  SD: File deleted");
    } else {
        Serial.println("  SD: Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        BHLOG(LOGSD) Serial.printf("  SD: %u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("  SD: Failed to open file for reading");
    }
    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("  SD: Failed to open file for writing");
        return;
    }
    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    BHLOG(LOGSD) Serial.printf("  SD: %u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}
