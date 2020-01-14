//*******************************************************************
// sdcard.h  
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// SDCard SPI related parameter file
//
//----------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at: 
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************

#define Select    LOW       //  Low  CS means that SPI device is Selected
#define DeSelect  HIGH      //  High CS means that SPI device is Deselected

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