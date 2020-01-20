//*******************************************************************
// wificfg.h  
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// Wifi network definitions
//
//----------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at: 
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
#ifndef WIFICFG_H
#define WIFICFG_H

// Replace with your network credentials
// or be Preprocessor definitions (-Dxxx)
// see also platformio.tpl

#define SSID_HOME  CONFIG_WIFI_SSID
#define PWD_HOME   CONFIG_WIFI_PASSWORD
#define HOSTNAME   "beeiot"

#define WIFI_RETRIES  10

#endif // end of wificfg.h
