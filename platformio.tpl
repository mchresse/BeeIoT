;PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[common_env_data]
lib_deps_builtin =
    WiFi        ; build in
    ESPmDNS     ; build in
    SD          ; build in
    WebServer   ; build in
    NTPClient   ; build in
    SPI         ; build in
    Preferences ; build in
    FS          ; build in
    Wire        ; build in

; lib_deps_external =
;    ArduinoJson@~5.6,!=5.4
;    https://github.com/gioblu/PJON.git#v2.0
;    IRremoteESP8266=https://github.com/markszabo/IRremoteESP8266/archive/master.zip

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

extra_scripts = pre:buildscript_versioning.py

; build_flags = 
build_flags = -DCONFIG_WIFI_SSID=\"MYDOMAIN\" -DCONFIG_WIFI_PASSWORD=\"MYPASSWD\"

; Library options
lib_deps =
    GxEPD       ; #2951
    DallasTemperature   ; #54
    RTClib      ; # 83
    U8g2        ; 942
    Adafruit GFX Library ; #13
    SdFat       ; #322
    HX711       ; #1100
    LoRa        ; #1167 Adafruit Sandeep Lora from https://github.com/sandeepmistry/arduino-LoRa
    OneWire     ; #1
    Adafruit ADS1X15 ; #342

; Serial Monitor Options
monitor_speed = 115200
;monitor_flags =
;    --encoding
;    hexlify

; Unit Testing options
test_ignore = test_desktop