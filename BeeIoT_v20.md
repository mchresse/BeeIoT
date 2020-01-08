# BeeIoT v2.0
### Eine Bienenstockwaage im Eigenbau (mit IoT Technik)
<img src="./images_v2/BeeLogFront.jpg" width=250>

**01.10.2019 by Randolph Esser**

---
## Inhaltsverzeichnis:

- [Einführung](#einführung)
	* [Gewichtsschwankungen pro Tag](#gewichtsschwankungen-pro-tag)
	* [Gewichtsschwankung pro Jahr](#gewichtsschwankungen-pro-jahr)
	* [Ereignisse](#ereignisse)
- [Das Modul-Konzept](#das-modul-konzept)
- [IoT für Bienen](#iot-für-bienen)
- [Das finale BeeIoT Client Modul](#das-finale-beeiot-client-modul)
- [Die MCU Arduino/ESP32 Platform](#die-mcu-arduino-ESP32-platform)
	* [Das ESP32-DevKitC Board](#das-esp32-devkitc-board)
	 	+ [ESP32 DevKitC Sockel Belegung](#esp32-devkitc-sockel-belegung)
	 	+ [ESP32 GPIO Nutzung](#esp32-gpio-nutzung)
- [Die BeeIoT Sensorik](#die-beeiot-sensorik)
	* [Die Wägezellen Auswahl](#die-wägezellen-auswahl)
	* [AD Wandler HX711](#ad-wandler-hx711)
	* [Temperatursensoren](#temperatursensoren)
	* [LED-Status Anzeige](#led-status-anzeige)
	* [SPI Devices](#spi-devices)
		+ [Micro-SDCard Modul](#micro-sdcard-modul)
		+ [LoRa WAN Support](#lora-wan-support)
		+ [NarrowBand-IoT](#narrowband-iot)
		+ [RTC Uhrzeit-Modul](#rtc-uhrzeit-modul)
- [Power Management](#power-management)
	* [Externer USB Port](#externer-usb-port)
	* [Batterie/Akku](#batterie-akku)
	* [Power Monitoring](#power-monitoring)
	* [PV Solar-Modul](#pv-solar-modul)
	* [Alternativer Batterie Laderegler](#alternativer-batterie-laderegler)
- [Der Aufbau](#der-aufbau)
	* [Stückliste](#stückliste)
	* [Das Aussengehäuse](#das-aussengehäuse)
		+ [Die Bosche Wägezelle](#die-bosche-wägezelle)
		+ [Das Anschluss Panel](#das-anschluss-panel)
			+ [One Wire Sensoren](#one-wire-sonsoren)
			+ [Der DS18B20 OW-Temperatur-Sensor](#der-ds18b20-ow-temperatur-sensor)
	* [Compute Node Box](#compute-node-box)
	* [Steckverbinder](#steckverbinder)

- [Die ESP32 BeeIoT Sketch Software](#die-esp32-beeiot-sketch-software)
	* [ESP32 Vorbereitungen](#esp32-vorbereitungen)
	* [ESP32 IDE](#esp32-ide)
		+ [PlatformIO](#platformio)
		+ [ESP32 Sensor Libraries + build](esp32-sensor-libraries-+-build)
	* [Die Programm Struktur](#die-programm-struktur)
		+ [Setup Phase](setup-phase)
		+ [Loop Phase](loop-phase)
	* [Kalibrierung der Waage](#kalibrierung-der-waage)
	* [Optional: WebUI Daten Service](#optional:-webui-daten-service)
- [WebUI ToDo - Liste](#webui-todo---liste)
- [Ideen Liste](#ideen-liste)
- [ToDo: Optional HW Module](#todo:-optional-hw-module)
	* [Nutzung einer SIM-Karte](#nutzung-einer-sim-karte)
- [Quellen/Links](#quellen/links)
	* [Marktübersicht professioneller Bienenstockwaagen](#marktübersicht-professioneller-bienenstockwaagen)
<!-- toc -->
---


## Einführung
Wie oft wünscht man sich als Imker, vor dem Standbesuch, zu wissen in welchem Zustand das Volk sich befindet, bevor man es öffnet. Dabei liegen die Vorteile auf der Hand:

+ Maßnahmen können vorher zeitlich eingeplant werden
+ Benötigtes Material kann für den Standbesuch vorbereitet werden
+ Die Behandlungszeit bei offener Beute kann ggfs. kürzer ausfallen, wenn sich die Mess-Annahme bestätigt

Interessante Parameter zum Bienen-Monitoring, die einem dabei behilflich sein können sind
+ **Der Beuten-Gewichtsverlauf**:
	- Honigverbrauch im Winter
	- Honigvorräte im Frühjahr (bis Trachtbeginn)
	- Anzeige von Honigverlust durch Räuberei in trachtarmer Zeit
	- Honigeintrag zur Haupt-Trachtzeit
	- Bedarf weiterer Honigräume
	- Zeitpunkt für Honigernte (Trachtende) bestimmen
+ **Die Stocktemperatur**
	- Temperaturverlauf im Bienenstock (Tag/Nacht) im Vergleich zur Aussentemperatur
		-> Anzeichen für die Aktivität des Volkes (besonders im Winter)
	- Beginn der Brutzeit durch Messung der Temperatur im Brutnest (>30° C)
	- Warnung bei Überhitzung des Bienenstocks (Gefahr für Brut und schweren Honigwaben)
	- Temperatur der Wintertraube (15-25 C) überwachen
	- Brutfreiheit für MS (Milchsäure) oder OS (Oxalsäure) Behandlung
+ **Fluglochaktivität** durch
	- Lichtschranke am Nesteingang
		+ Räubereialarm (Kämpfe am Nesteingang)
		+ Schwarmalarm bei viel Aktivität am Nesteingang in sehr kurzer Zeit (30Min.) 	 korrespondierend mit spontanem Gewichtsverlust (Delta: ca. 1,5 kg)
+ **Örtliche Wetterdaten**

Das Ziel wäre dann aus diesen Daten Schlussfolgerungen für den nächsten Standbesuch zu ziehen oder die Wirkung letzter Massnahmen zu verfolgen:
+ **Futterzustand** von Einwinterung E9 bis zur Auswinterung im Frühjahr A4
+ **Brut-Aktivität** durch ganzjährigen Temperaturverlauf im Bienenstock
	- Brutende zum Jahresende			-> Möglichkeit zur Varroabehandlung
	- Brutbeginn zum Jahresanfang 		-> "Bee-Alive" Status des Volkes
+ **Honigeintrag** und Trachtende zu den jeweiligen Trachtphasen
+ **Schwarmabgang** („bei dem Event weiss ich, ich war zu spät“)

Weitere optionale Sensor-Module:
+ Mit der *Infra-Rot Kamera* von außen auf die Gesamtbeute gerichtet:
  Liefert Hinweise auf den Bienensitz, und die Populationsaktivität im Winter
+ Mit dem *Fluglochzähler*: Flugaktivität am Flugloch

Um die anfallenden Sensordaten auswerten zu können muss ein gewisses *Referenz-Verhalten* an jedem Sensor angenommen werden um Abweichungen/Fehlmessungen zu erkennen (== Anomalie-Erkennung ==).
So gibt es allgemeine Gewichtsverläufe über das ganze Jahr hinweg, aber auch innerhalb eines Tages, die man recht gut bestimmten Volks-Zuständen zuordnen kann:

### Gewichtsschwankungen pro Tag
Hier nun ein idealisierter qualitativer Gewichtsverlauf eines sonnigen Sommertages (=Flugwetter):

<img src="./images_v2/WeightDayChart.jpg">

Der Tag beginnt mit dem Ausflug der Bienen (ca. 1-2kg). Da eine Biene etwa 0.1 Gramm wiegt und ca. 10-15.000 Flugbienen möglich sind ergibt sich somit ein Delta von 1-1,5kg. Diese kehren in den folgenden Stunden immer wieder mit Honig, Pollen und Wasser zurück, so dass das Gewicht kontinuierlich steigt. 
Bis zum Sonnenuntergang ist die „Rückkehr“ der Sammlerinnen abgeschlossen und die Honigtrocknung durch die Stockbienen beginnt (begünstigt durch die kühleren Nachttemperaturen). 
Das Delta stellt den Tageseintrag dar.

### Gewichtsschwankung pro Jahr
Über das Jahr wirken die verschiedenen Maßnahmen an den Völkern auf die Gewichtskurve ein und lassen ggfs. Erfolg (z.B. Honigeintrag) oder Misserfolg (z.B. keine Futterannahme) kontrollieren.

<img src="./images_v2/WeightYearChart.jpg">
Natürlich ist der Verlauf stark von den individuellen Behandlungs-Konzepten des Imkers abhängig, weshalb in den Stockwaagendiagrammen an jedem Änderungspunkt die entsprechende Massnahme markiert ist, um den Kurvenverlauf besser interpretieren zu können. So würde eine totale Brutentnahme im Sommer nach der letzten Schleuderung einen deutlich anderen Spätsommerverlauf haben; ebenso eine 3. Schleuderung bei Spättracht-Imkerei.

Spätestens bei der letzten Einfütterung und Einwinterung sind aber wieder alle Verläufe gleich zu erwarten.
Der tageweise Honigeintrag der Früh- und Sommertracht ist, wie aus dem oben dargestellten Tagesverlauf abgeleitet, durch den Treppenverlauf markant zu erkennen.

Ebenso auffällig sind die Ruhephasen eines Volkes nach/während der AS Behandlung. Diese werden aber teilweise durch die allgemeine Volksumstellung auf Winterbrut überlagert.

### Ereignisse
Sonderereignisse wie
+ Schwarmabgang
+ Schneefall
+ Plünderung/Diebstahl (:()
+ Ggfs. Räuberei

sind in Abweichung zu den erwarteten Referenzverläufen ebenfalls unter Berücksichtigung der akt. Wetterlage erkennbar.

In der Regel reicht die Stockwaage nur an einem Referenzvolk pro Standort um Globalereignisse wie der richtige Schleuderzeitpunkt für alle Völker am Trachtende zu ermitteln.

Im Weiteren stelle ich nun die Funktionen der Stockwaagemodule vor.

## Das Modul-Konzept
Dieser Aufbauvorschlag besteht im Kern als Mini-Compute Node aus einem Raspberry Pi für den Ablauf das Logger Programms, einer (!) Bosche Wägezelle zur Gewichtsmessung und diverse OneWire Sensoren für verschiedene Temperaturmessungen sowie weiterem Mess-Zubehör.

Der Compute Node wird über einen Mini-USB Port mit Strom versorgt und mit FW Updates versorgt.
Optional kann über ein LAN Kabel die Kommunikation betrieben werden und über POE (Power over Ethernet) eine weitere Stromquelle erschlossen werden.
Im weiteren kann die Kommunikation über die Protokolle WiFi, BLE, GPRS betrieben werden.

Als redundante Stromquelle zum USB Port dient eine Lithium-Batteriespeisung mit Aufladung einerseits über POE oder alternativ über ein externes PV Falt-Modul. Letzteres wird für mobileren Einsatz benötigt.

Die Messdaten werden über alle Sensoren erhoben, in Datentabellen auf der SD-Karte abgespeichert und per NB-IoT/WiFi per MQTT/FTP zur Darstellung und/oder Auswertung z.B. an eine extern hosted Web Page übertragen.
Parallel zu dieser externen Ablage stehen die Webpage files auch über den lokalen Webserver des Raspberry als Edge Server selbst zur Verfügung.
Neben der einfachen Messdatensammlung dienen diverse Algorithmen auf dem Edge Server zur Plausibilisierung der Messwerte und statistischer Aufbereitung für das Tages-, Wochen- und  Monats-Mittel, welche damit ebenfalls als Diagramm aufbereitet dargestellt werden können. Der ESP32 liefert "nur" die rohen Messwertdaten.

<img src="./images_v2/BeeLogConcept.jpg">

## IoT für Bienen
Der obige Fat-Client Ansatz dient als POC - ProofOfConcept für alle Sensor-Algorithmen und grundsätzliche (Ver-) Messbarkeit von Vorgängen in einer Bienenbeute.

Deutlich eleganter und damit skalierbarer ist der Modulstack im IoT Aufbau:

<img src="./images_v2/BeeIoT_Concept.jpg">

Die eigentliche Aufgabe der Sensorsteuerung und Roh-Messdatenaufnahme kann auch mit einer Arduino kompatiblen MCU (ESP32, XMC, ...) geleistet werden:

**Vorteil:**
+ geringerer Stromverbrauch pro Sensorclient Modul
+ geringere Invest-Kosten
+ übersichtlichere Programmstruktur durch entzerrten Modulstack
+ Skalierbarkeit durch standardisierte Schnittstellen 
	+ -> eine gesicherte Übertragung ist leichter realisierbar
+ Dadurch sind nahezu beliebig viele Sensor Clients in Reichweite verwaltbar.

**Nachteil:**
+ eine weitere Instanz zur Datenaufnahme
+ Mindestens 2 neue zu sichernde remote Kommunikationskanäle (NB-IOT/LTE + REST/Cloud service)
+ Dadurch höhere Kosten für mehr Remote Übertragungswege (-Verträge)

Grundsätzlich dient das Edge Device zur Registrierung und Kontaktierung der Sensor Clients via MQTT. Dabei findet eine  Konsolidierung der Rohdaten auf das wesentliche zur Übertragung an einen Cloud-/Web Service statt.
Erste Plausibilitätschecks und Tag-/Monat-/Jahres-Statistiken können ebenfalls erstellt werden.
Denn dafür reicht die Power eines Raspi Zero o.ä. bei Weitem.

Dies als Ausblick für die noch im Bau befindliche v2.0 Verion. Die Sensorlogik und Dateninterpretation zur Völksführung ist aber weitgehend synonym zur v1.x Version.


## Das finale BeeIoT Client Modul
Nachdem nun die funktionalen Anforderungen klarer umrissen sind, geht es an die Modulplanung:

<img src="./images_v2/BeeIoT_Client_Moduleplan.jpg">

Das vollständige Übersichtsbild der Stockwaagenmodule zeigt die Vielfalt der verwendeten Techniken, aber auch das Potential in Sachen Spannungsversorgung und Sensorik:

Folgende Funktionen wollen wir unterbringen:
+ Spannungsversorgung
	- Für das MCU Extension board:
		* Interner Lithium Akku (Laufzeit 6 Monate) mit Ladekontrollmodul (BMS)
	 	* über einen Spannungsanheber/Senker (BUKBooster) mit 5V zur internen USB Buchse der MCU
	 	* Akku von Extern ladbar via 5V USB Buchse, über die aber auch FW udate betrieben werden kann
	- PV Modulanschluss extern -> ebenfalls über den externen 5V USB Anschluss möglich zur Verlängerung der Akkulaufzeit
	- Optional: POE via RJ45 LAN-Hat mit POE/LAN Splitter
+ Kommunikation
	- Übertragung aller Sensordaten alle 10 Minuten um auch "Bienen-Events" mitzukriegen
	- lokal via externen USB auf internen USB/Ser. Adapter Port der MCU
	- WiFI support mit HTTP Web Service zur Startup Configuration
	- LoRaWAN Kommunikation mit Gateway (bis zu 7km Radius)
	- optional: Bluetooth (BLE)
	- Optional: NB-IOT via SIM7000E GPRS Modul
+ Sensorik
	- 100kg Wägezelle über einen 24bit A/D Wandler Modul HX711(am Raspi GPIO Port Anschluss)
	- OneWire Temperatur Sensor 12bit intern an der Wägezelle (zur temp. Kalibrierung)
	- OneWire Temperatursensor 12bit extern
	- OneWire Temperatursensor 12bit extern für Stocktemperatur Messung (über 1m Leitung)
+ Optionale Erweiterung
	- USB GPS Maus (als Diebstahlschutz)
	- IR Kameramodul extern (USB oder I2C Port Anschluss)
	- IR Lichtschranke als Fluglochzähler (beecounter)

Daraus ergeben sich spezielle technische Anforderungen an jedes modul bezüglich Stromverbrauch, benötigte Schnittstellen und lokale/remote Erreichbarkeit, die wir weiter unten noch zu diskutieren haben.

## Die MCU Arduino/ESP32 Platform
Um bis zu 6 Monat Laufzeit zu erreichen darf  man bei einem  10.000mAh Akku nur im Mittel 2.3mA verbrauchen (10.000/4320). Das schafft man bei so vielen Sensoren/Modulen aber nicht im Dauerlaufzustand, sondern nur indem man Sleep Phasen einschiebt.

*Dazu folgende Rechnung:*
Unter der Annahme man benötigt für die Ermittlung der Sensordaten inkl. Übertragung z.B 10 Sekunden,
Wäre das Verhältnis aktiver Betriebszeit zu Sleep Phase: 10/(6*30*24*60*60)=1/1.555.200.
Würde man in der SleepPhase aber unter 1mA kommen hätte man für die Betriebsphase über die gesamte Laufzeit noch zur Verfügung:
- Anteil Aktivphase: 10 Sek./10Minuten: 1/60
- auf 6 Monate entspricht das (6*30*24)h/60 = 72h = 3Tage Aktivphase
- => 4320-3 Tage Passivphase = 4317 Tage
- Stromverbrauch Passivphase: 4317 * 1mAh = 4317mAh von 10.000mAh.
- Möglicher **Stromverbrauch Aktivphase**: (10.000mAh - 4317mAh)/72h = 5683mAh / 72h = **~80mA**

Bezüglich Stromverbrauch kommt unterhalb des RaspberryPi Zero, der i.d.Regel >150mA liegt nur noch die Arduino Klasse in Frage, mit seinen bekannten Vertretern UNO 8266 und ESP32 von Espressif:

### Das ESP32-DevKitC Board
Als kompakteste MCU Variante mit sehr niedrigem Stromverbrauch, grosser Anzahl GPIO Ports  und weitverbreiteter Sensor-Unterstützung kommt hier der Arduino kompatible ESP32 in Form des ESP32-DevKitC zum Einsatz.

Der ESP32 unterstützt folgende APIs:
+  3x UART (RS232, RS485 und auch IrDA)
+  4x SPI (SPI0/1 for RD/WR flash Cache only; HSPI and VSPI free in Master/Slave Mode)
+  2x I2C (Std: 100 KBit/s und Fast-Mode (400 KBit/s))
	+ 2x GPIO ports - SW konfigurierte IO pins via *i2c_param_config()*
+  1x CAN Bus v2.0
+ 18x 12bit AD Converter
+  2x 8bit DA Converter
+  1x HW-PWM
+ 16x SW-PWM
+  1x IR Controller
+  4x 64bit Timer
+  1x int. HALL sensor
+  1x int. Temperatur Sensor (40-125 Grad F.)
+  Secure Boot Mode -> erlaubt das *Root of Trust* Konzept durch den "eFuse Speicher".

Zwar zeichnet sich ein Arduino 8266/UNO durch geringeren Stromverbrauch aus, der ESP32 hat aber mehr GPIO Leitungen, die wir dringend für alle verwendeten Module benötigen.

Allerdings kommt hier nicht die standardmäßig verbaute **Wroom32** Version des ESP32 zum Einsatz, sondern der **Wrover32**:

<img src="./images_v2/ESP32_WROOM-32D.jpg"> ==> <img src="./images_v2/ESP32_Wrover-B_2.jpg">

Das ergibt die genaue Modul-Bezeichnung: **ESP32-DevKitC-Wrover-B**.

Der **Wrover-B** hat neben dem 4MB Flash Memory auch noch einen zusätlichen 8-16MB PSRam Bereich, indem wir temporär anfallende residente Sensordaten ablegen/puffern können.
Ansonsten sind sie Pin- und Code-Kompatibel so dass grundsätzlich auch der Wroom verwendet werden kann. Die Nutzung des PSRam erörtern wir später.

Die Variante DevKit-C bringt bereits einige wichtige Betriebsfunktionen mit sich und ist durch ihr Standardsockelformat leichter auf ein 2,4" Lochrasterboard zu löten.
OnBoard findet sich auch bereits eine Bluetooth (4.2 + BLE) und WiFI (IEE 802.11 b/g/n) Unterstützung mit onboard Antenne. Zur Reichweitenvergrößerung ist bei manchen Ausführungen ein weiterer Antennen-Stecker vorhanden, an den eine zusätzliche externe WiFI Antenne angeschlossen werden kann.

#### ESP32 DevKitC Sockel Belegung
Zunächst das Standard-Pinning und die Default-Functional Overlays des DevKit C boards, wie sie durch den Microcode bei Power-ON voreingestellt sind:

|         |         |      |    |PIN|*|PIN|     |      |         |       |
|---------|---------|------|----|---|-|---|-----|------|---------|-------|
|         |         |      |3.3V|  1|*| 38|  GND|      |         |       |
|    SW2  |         |      |  EN|  2|*| 37| IO23|GPIO23|VSPI-MOSI|       |
| Sens-VP |ADC1-0   |GPIO36|  SP|  3|*| 36| IO22|GPIO22|I2C-SCL  | ->Wire|
| Sens-VN |ADC1-3   |GPIO39|  SN|  4|*| 35|  TXD|GPIO01|UART0-TXD| ->USB |
|         |ADC1-6   |GPIO34|IO34|  5|*| 34|  RXD|GPIO03|UART0-RXD| ->USB |
|         |ADC1-7   |GPIO35|IO05|  6|*| 33| IO21|GPIO21|I2C-SDA  | ->Wire|
|  XTAL32 |ADC1-4   |GPIO32|IO32|  7|*| 32|  GND|      |         |       |
|  XTAL32 |ADC1-5   |GPIO33|IO33|  8|*| 31| IO19|GPIO19|VSPI-MISO|       |
|    DAC1 |ADC2-8   |GPIO25|IO25|  9|*| 30| IO18|GPIO18|VSPI-SCK | U0-CTS|
|    DAC2 |ADC2-9   |GPIO26|IO26| 10|*| 29| IO05|GPIO05|VSPI-SS  |       |
|         |ADC2-7   |GPIO27|IO27| 11|*| 28| IO17|GPIO17|UART2-TXD|       |
|HSPI-SCK |ADC2-6   |GPIO14|IO14| 12|*| 27| IO16|GPIO16|UART2-RXD|       |
|HSPI-MISO|ADC2-5   |GPIO12|IO12| 13|*| 26| IO04|GPIO04|HSPI-4HD | ADC2-0|
|         |         |      | GND| 14|*| 25| IO00|GPIO00|  SW1    | ADC2-1|
|HSPI-MOSI|ADC2-4   |GPIO13|IO13| 15|*| 24| IO02|GPIO02|HSPI-4WP | ADC2-2|
|  U1-RXD |FLASH-D2 |GPIO09| SD2| 16|*| 23| IO15|GPIO15|HSPI-SS  | ADC2-3|
|  U1-TXD |FLASH-D3 |GPIO10| SD3| 17|*| 22|  SD1|GPIO08|FLASH D1 | U2-CTS|
|  U1-RTS |FLASH-CMD|GPIO11| CMD| 18|*| 21|  SD0|GPIO07|FLASH D0 | U2-RTS|
|         |         |  Vin | +5V| 19|*| 20|  CLK|GPIO06|FLASH CLK| U1-CTS|
**Pinning durch IDE board pre-selection = "esp32dev" für ESP32 DevKit-C boards **

Welche default Einstellung pro Board bei der IDE gilt kann man bei Platformio z.B. hier finden:
C:\Users\'benutzername'\.platformio\packages\framework-arduinoespressif32\variants\'boardtype'\pins_arduino.h

Als nächstes macht man sich auf die Suche nach geeigneten GPIO Control Leitungen, die mit ihren Eigenschaften der angeschlossenen Funktion für den Modulanschluss entsprechen müssen. Manche GPIO Leitungen sind, anders als bei einem reinen ESP32 Modul bereits durch onboard Funktionen des DevKitC Boards belegt. Dies muss aber kein Nachteil sein, weil die Funktionen ja auch benötigt werden:

Geflasht wird der ESP32 über die onboard USB-A Buchse die auch gleichzeitig zur 5V Stromversorgung des DevKitC Boards verwendet wird (standard). Onboard wird dann die 3.3V Leitung für die ESP32 MCU über einen internen DC/DC Wandler AMS1117-3.3 von Microship erzeugt, der max. 1A durchsetzen kann.
Zur USB/Ser.Kommunikation sind daher die Leitungen GPIO 1+3 (RX+TX)belegt.

Ein besonderes Augenmerk ist bei den Logikanschlüssen auf die richtige Logiklevel Nutzung bei unterschiedlich verwendeten Stromversorgungen der Sensormodule aufzubringen, wie sich später noch zeigen wird...(s. Micro SD-Card Modul)

**Hinweis: Der ESP32 selbst und alle GPIO-Anschlüsse arbeiten aber ausschliesslich mit 3.3V.**

Für den Flash-MM Anschluss werden GPIO6-11 onboard verwendet und stehen für unser Projekt daher nicht zur Verfügung. Auch die Leitungen EN(Boot-Button) und GPIO36(SP) + 39(SN) sind bereits belegt.
GPIO-00 wird per DevKitC FW als Reset Interrupt ausgewertet.

Über ein USB Kabel an einen Windows PC angeschlossen, zeigt sich der "Onboard USB to serial Converter" CP2102N im Windows Device Manager als:
<img src="./images_v2/ESP32_COMPort.jpg">

Über dieses COM device ist er auch für IDEs wie die *Arduino-IDF* (https://github.com/espressif/esp-idf) oder *VC-Platform-IO* erreichbar. Eine COM Port Erkennung findet zumindest bei PlatformIO automatisch statt.

Auf einem Linux System nennt sich das USB Converter Device: */dev/ttyUSB0*.

Als Standard Baudrate sollte man es erstmal mit 115200 versuchen.
Der USB/UART Converter CP2102 schafft aber max. 921600 Baud rate.

#### ESP32 GPIO Nutzung
Und nun konkret die gewählte GPIO-Belegung der ESP32 DevKitC Sockel-Pins für das BeeIoT Projekt:
(gilt für WROOM32D alsauch Wrover-B Module)

|Pin| Ref. |  IO# |  DevKitC |       | Protocol |  Components    |
|---|------|------|----------|-------|----------|----------------|
|  1| +3.3V|      |          |       |    +3.3V | 3.3V           |
|  2|    EN|      |   SW2    |       |          | ePaper-Key2    |
|  3|*  SVP|GPIO36|  Sens-VP | ADC1-0|        - | x              |
|  4|*  SVN|GPIO39|  Sens-VN | ADC1-3|        - | x              |
|  5|* IO34|GPIO34|          | ADC1-6|          | ePD-K3/LoRa DIO2|
|  6|* IO35|GPIO35|          | ADC1-7|          | ePaper-Key4    |
|  7|  IO32|GPIO32|   XTAL32 | ADC1-4|OneWire-SD| DS18B20(3x)    |
|  8|  IO33|GPIO33|   XTAL32 | ADC1-5|          | LoRa DIO0      |
|  9|  IO25|GPIO25|     DAC1 | ADC2-8|  Wire-DT | HX711-DT       |
| 10|  IO26|GPIO26|     DAC2 | ADC2-9|  Wire-Clk| HX711-SCK      |
| 11|  IO27|GPIO27|          | ADC2-7| ADS-Alert| ADS1115/BMS    |
| 12|  IO14|GPIO14| HSPI-CLK | ADC2-6|Status-LED| LoRa RST       |
| 13|  IO12|GPIO12| HSPI-MISO| ADC2-5| SPI1-MISO| LoRa CS\       |
| 14|   GND|      |          |       |       GND| GND            |
| 15|  IO13|GPIO13| HSPI-MOSI| ADC2-4| SPI1-MOSI| LoRa DIO1      |
| 16|   SD2|GPIO09|FLASH-D2  | U1-RXD|        - | x              |
| 17|   SD3|GPIO10|FLASH-D3  | U1-TXD|        - | x              |
| 18|   CMD|GPIO11|FLASH-CMD | U1-RTS|        - | x              |
| 19|   +5V|      |          |       |  +5V_Ext | n.a            |
| 20|   CLK|GPIO06|FLASH-CLK | U1-CTS|        - | x              |
| 21|   SD0|GPIO07|FLASH-D0  | U2-RTS|        - | x              |
| 22|   SD1|GPIO08|FLASH-D1  | U2-CTS|        - | x              |
| 23|  IO15|GPIO15| HSPI-SS  | ADC2-3|MonLED-red| Red LED        |
| 24|  IO02|GPIO02| HSPI-4WP | ADC2-2|  SPI-CS  | SDcard CS\     |
| 25|  IO00|GPIO00|   SW1    | ADC2-1|          | ePaper-Key1    |
| 26|  IO04|GPIO04| HSPI-4HD | ADC2-0|      CLK | ePaper BUSY    |
| 27|  IO16|GPIO16|UART2-RXD |       |      DT  | ePaper RST     |
| 28|  IO17|GPIO17|UART2-TXD |       |          | ePaper D/C     |
| 29|  IO05|GPIO05| VSPI-SS  |       | SPI0-CS0 | ePD CS\        |
| 30|  IO18|GPIO18| VSPI-CLK |       | SPI0-Clk | ePD/SD/LoRa Clk|
| 31|  IO19|GPIO19| VSPI-MISO| U0-CTS| SPI0-MISO| SD/LoRa MISO   |
| 32|   GND|      |          |       |      GND | GND            |
| 33|  IO21|GPIO21| VSPI-4HD |       |  I2C-SDA | ADS1115/BMS    |
| 34|  RXD0|GPIO03|UART0-RX  | U0-RXD| -> USB   | USB intern     |
| 35|  TXD0|GPIO01|UART0-TX  | U0-TXD| -> USB   | USB intern     |
| 36|  IO22|GPIO22| VSPI-4WP | U0-RTS|  I2C-SCL | ADS1115/BMS    |
| 37|  IO23|GPIO23| VSPI-MOSI|       | SPI0-MOSI|ePD/SD/LoRa MOSI|
| 38|   GND|      |          |       |       GND| GND            |
(x unter components => darf nicht verwendet werden.)

Das MCU board erhält die 5V versorgung über den internen USB connector und erzeugt die 3.3V selbst.
Der 5V_Ext Anschluss wird nicht verwendet. Die 5V Leitung des MCU Extension Boards wird direkt von der Batterieversorgung gespeist.

Damit sind alle möglichen GPIO Leitungen funktionell belegt. Eine Erweiterung wäre nur noch über aufwendige IO port Multiplexer möglich. Die Auswahlkriterien werden weiter unten nochmal pro Sensor diskutiert.

## Die BeeIoT Sensorik
Als IoT Sensoren werden folgende Elemente und Anschlüsse verwendet:
+ 1x Wägezelle	-> über einen 24bit A/D Wandler HX711 an die GPIO Ports des ESP32 angeschlossen.
+ 3x Temperatursensoren zur Messung der (DS18B20 OneWire)
	- Intern: Stocktemperatur (innerhalb der Beute)
	- Externe Temperatur
	- Wägezellen Temperatur (ggfs. zur Kompensation einer Temperaturdrift)
	- => Alle Temp. Sensoren sind über das OneWire Protokoll direkt an einen GPIO Port des ESP32 angeschlossen.
+ 1x ADS1115 Messung des Batteriespannungspegel, Converter Ausgang (5V) sowie Ladespannung
+ 1x RTC Modul (I2C)(DS3231) incl. Eprom
+ 1x Micro SD Card Modul (SPI)
+ 1x ePaper 2.7" (SPI) + 4 F-Keys.
+ 1x LoRa MAC Modul (Dragino LoRa Bee 868MHz)
+ **Optional**
	- 1x xternes IR-Kameramodul -> für Bienensitz im Winter
	- NB-IoT Modul zur Funk-Fernübertragung via GPRS (SM7000E)
	- USB GPS Maus (mapped über ser. Port RX/TX)

### Die Wägezellen Auswahl
Eine Standardwägezelle mit geringer Temperaturdrift und hoher Gewichtsstabilität sowie Messwiederholgenauigkeit weist heutzutage typischerweise vorkalibrierte Dehnmesstreifen in einer Wheatstonebrücke verschaltet auf. Diese ist eine gegenläufige Verschaltung von 4 Dehnmessstreifen (DMS) im Rautenmodell (siehe Schaltbild weiter unten) incl. Temperaturdriftkompensation.

Meine Auswahl fiel (wie bei so manchen anderen Waagen-Projekte im Netz auch) auf den Hersteller Bosche. Die Wägezelle muss folgenden Anforderungen entsprechen:
+ Messbereich 100Kg (Waagendeckel + Beute (2 Brut- und 2 Honig-Zargen) + Deckel und Abdeckgewicht.
+ Stabile Verschraubung mit dem Waagendeckel zur Kraftübertragung möglich (für Zander: 40 x 50cm) Unterboden
+ Dadurch geringe Eckgewichtslast-Fehler
+ Spannungsversorgung ab 5V (aus Raspi Versorgungsspannung)
+ Großer Temperaturmessbereich -30 … +50 Grad
+ bei vorgegebener Genauigkeit: Class C3: 2mV/V
	- Bei  5V Messzellenspannung ergibt das eine Messabweichung von
(100kg/5V)*2mV => +/- 40Gramm
+ Wasserfestigkeit IP65 (spritzwassergeschützt)

Daher fiel die Wahl auf die: **Bosche H40A Wägezelle** :
<img src="./images_v2/WeightCell.jpg">
mit den Eigenschaften
+ Nennlast 100 kg
+ Kompensierte Eckenlastfehler
+ Für Dauereinsatz geeignet
+ Bezug direkt über den Bosche Shop: http://www.bosche.eu
+ Material: Aluminium
+ Genauigkeitsklasse bis C4 (C3 Standard, C4 auf Anfrage), Y=15.000, Nennwert-Toleranz: 2,0 mV/V
+ Eichfähig nach OIML R60 bis 4000D, Prüfscheinnummer: DK0199-R60-12.19
+ Aufbau: Das Messelement ist vergossen, Schutzklasse: IP65
+ Max. Plattformgröße: 500 x 500 mm
+ Stromversorgung: 5 – 12V
+ Temperarturbereich: - 30 ... + 70 °C
+ Anschluskabel: 1,8m
+ Preis: 58€

Anfangs habe ich mit dieser wesentlich günstigeren Wägezelle gestartet: --YZC161E-- (ca. 4,50€/Zelle)
Durch den Aufbau bedingt verkraftet diese Zelle aber nur 50Kg pro Zelle und ist baulich als Auflagesensor gedacht. Daher werden 4 Stück an jeder Ecke benötigt, die als Wheatstone-Brücke verschaltet werden müssen.

Wie aus dem unteren Bild (linke Hälfte) aber ersichtlich weisen die Auflagepunkte eine Reibung auf (je mehr Gewicht desto höher), die bei einer horizontalen Temperatur-Ausgleichsbewegung der Stabilisierungsträger Verspannungen hervorruft, und somit die Messzelle und damit die DMS vorspannt und zu starken Messfehlern führt.
Besonders unangenehm ist das „Springen“ der Auflage über den Messzellenauflagepunkt ab Überschreitung einer max. Dehnungs-Spannung, was wiederum zu Messwertsprüngen führt. Diese lassen sich auch über eine Messzellen-Temperaturmessung oder algorithmisch nicht mehr verlässlich kompensieren.
Diese Wägezelle eignet sich vorrangig für Körperwaagen, wo i.d.R. nur spontane Einzel-Messungen mit Rücksprung auf das Null-Gewichtslevel erfolgen. Für konstante Dauermessungen auf höherem Gewichtsniveau eignen sie sich nach meinen Erfahrungen nicht.

<img src="./images_v2/WeightCellDesign.jpg">

Daher habe ich diese zwar günstige aber aufwändig installierbare und nicht kalibrierbare Variante wieder verworfen.

Die Anbindung der **Bosche Wägezelle H40A** an den ESP32 erfolgt über das 24bit A/D-Wandler Modul ‘HX711‘. Dieses wurde speziell für Wägezellen mit Wheatstone-Brücke konzipiert und weisst eine einstellbare Verstärkung und integrierte Referenz-Spannungsversorgung auf.
Beim Kauf ist darauf zu achten, dass die Module an den Anschlüssen E- bzw. BLK mit GND verbunden sind, was sich sonst in einer geringeren Temperaturstabilität und Meßstreuung auswirkt. Im Zweifel muss die Draht-Brücke selbst durch einen externen Draht nachgearbeitet werden.
Module mit grüner Schutzfarbe sind aber i.d.R. richtig beschaltet.
Desweitern werden die Wägezellen durch ihre Anzahl an Dehnmessstreifen (DMS) mit einer Wheatstonebrücke versehen. Ein DMS wandelt eine Gewichtsbelastung an einer Körperoberfläche in Widerstandsänderungen der Größenordnung Faktor 0,0001 – 0,001 eines Referenzwertes (hier ca. 400 Ohm) um. Diese sehr geringe Änderung ist durch eine Messbrücke nach Wheatstone zu vermessen, welche durch ihre gegenläufigen Widerstandspaare temperaturstabilisierend und messverstärkend wirkt.

In der Bosche H40A sind die DMS derart verschaltet, dass das Ausgangssignal direkt auf den A/D Wandler HX711 geführt werden kann, was den Aufbau stark vereinfacht.

### AD Wandler HX711
Der AD Wandler HX711 ist für die Messung von Wheastonebrücken optimiert und bietet dazu 4 Anschlüsse E+/- + A+/-. An den Pins B+/B- liegt die Messspannung für die Waagzellenbrücke an.
In unserem Fall ist es die 3.3V Linie von der ESP32 MCU kommend. Die 5V Versorgung, welche aufgrund des weiteren Spannungsfensters einen geringeren Messfehler liefern würde, kann leider nicht genommen werden, da die zugehörigen Logikleitungen nur 3.3V aufweisen dürfen.

Hier könnte man ggfs. mit einem weiteren 5V <-> 3.3V Logikconverter optimieren.

Auf der rechten Seite befinden sich 4 Anschlüsse zur digitalen Anbindung an die GPIO Ports des ESP32:
<img src="./images_v2/HX711.jpg">


Die Eigenschaften des HX711:
+ 2 wählbare Eingänge zur Differenzmessung
+ Eine On-chip active rauscharme Steuereinheit mit wählbarer Verstärkung (32, 64 und 128)
+ On-chip Stromversorgungskontrolle für die Waagzelle
+ On-chip Rest nach Einschaltung
+ Einfache digitale Anbindung
+ Wählbare Samplegeschwindigkeit (10SPS oder 80SPS)
+ Stromsparregler: normaler Betrieb < 1.5mA, Off Mode < 1uA
+ Spannungsversorgungsbereich: 2.6 - 5.5V mit enstellbarer Verstärkung
+ Temperaturbereich: -40 - +85?

Für Kanal A kann eine Verstärkung von 128 oder 64 gewählt werden, Kanal B bietet eine fixe Verstärkung von Faktor 32. Daher habe ich für diese Version nur den Port A mit GAIN 128 verwendet und Port B stillgelegt.
Zum Anschluss der HX711 Logik-Ports an den ESP32 werden nur 2 der generischen duplex-fähigen GPIO Ports benötigt 
(Data + SCK):
```cpp
#define HX711_DT    25    // serial dataline
#define HX711_SCK   26    // Serial clock line
```
Zur Stabilisierung und Entstörung der Stromversorgungsleitungen (3.3V & 5V) werden je 1x 100nF Siebkondensatoren sowie je ein 4,7uF Puffer-Elko verwendet.
Zur Abwägung dieses Stabilisierungsaufwandes dient folgende Genauigkeitsbetrachtung:

+ Gegeben: Speisespannung der Wheatstonebrücke von 3.3 Volt und einer Nennwert-Toleranz von 2mV/V (Class C3)
+ Bei 5V Messzellenspannung ergibt das eine **Messabweichung** von (100kg/3.3V) x 2mV/V => **+/- 60 Gramm**
+ Das ergibt eine **Empfindlichkeit** von 3.3V x 2mV/100kg = **66µV/kg = 66nV/Gr**
+ Als default verstärkung (GAIN) wird 128 angenommen.

Der verwendete A/D Wandler hat aber eine maximale Spannungsauflösung/bit von 128 x 3.3V/2hoch24 =**~1,5 nV** bei einer Vertärkung von 128 an Port A. Dies ergibt den Wert von 66nV/Gr / 1,5nV = **44**/Gramm.
Als 1Kg-Scale-Divider müsste man im Programm also später den Wert **44000/kg** ansetzen. Der Scale Offset Wert muss durch die Vermessung des äußeren Aufbaus (Deckel) erst ermittelt werden.

Der HX711 ist mit 24bit Genauigkeit auch an nur 3.3V also mehr als hinreichend ausgelegt. Diese Genauigkeit Bedarf aber einer gut stabilisierten Betriebsspannung.
Die Streuung um die 3.3V Stromversorgung bestimmt also direkt die Qualität/Streuung der Messwerte.
Der daraus resultierende Messfehler durch die Streuung kann aber über Mittlung von mehreren (~10-20) Messwerten deutlich verringert werden und damit den Vertrauensbereich deutlich erhöhen.

Im Arduino Sketch ist der HX711 Anschluss wie folgt definiert:
```cpp
// Library fo HX711 Access
#include <HX711.h>

// HX711 ADC GPIO Port to Espressif 32 board
#define HX711_DT    25    // serial dataline
#define HX711_SCK   26    // Serial clock line
#define scale_DIVIDER 44000   // Kilo unit value (2mV/V, GAIN=128 Vdd=3.3V)
#define scale_OFFSET 243000   // 243000 = 44000 * 5.523kg (of the cover weight)
```

### Temperatursensoren
Für den Anschluss von mehreren Messeinheiten über einen (3-pol.) Eingangsport bietet sich das serielle OneWire Protokoll an. Dafür steht eine große Palette an günstigen (Temperatur-)Sensoren zur Auswahl.

Für die benötigte Genauigkeit der Temperaturmessung (9-12bit) wird der Sensor DS18B20 von Dallas verwendet, den es in der Bauform als 1,8m langes Versorgungskabel (3-pol.) und IP67 Abdichtung mit einer robusten Aluminium-Messspitze als Messsonde gibt.

Wie aus dem Bild ersichtlich, werden einfach alle OneWire Sensoren mit ihren 3 pol. Leitung parallel angeschlossen (es sind bis zu 10 Sensoren möglich). Zur Pullup-Versorgung der Datenleitung ist ein Widerstand von 4.7k Ohm gegen 3.3V einmalig für alle angeschlossenen Sensoren nötig, da alle Sensoren alsauch der ESP32 GPIO port mit OpenCollector Treibern arbeiten. der Pullup schafft damit einen definierten Logikpegel.

<img src="./images_v2/BeeIoT_Schematics.jpg">

Im Arduino Sketch finden sich dazu folgende Einstellungen:
```cpp
#include "OneWire.h"
	#include "DallasTemperature.h"
	// Data wire is connected to ESP32 GPIO 32
	#define ONE_WIRE_BUS 32
```


### LED-Status Anzeige
Zur Anzeige des Betriebszustandes dient eine rote LED am rückseitigen externen Anschlusspanel.

Diese wird über einen GPIO Port getrennt angesteuert und zeigt durch Blinkcodes verschiedene Programm- Zustände (Setup / Loop / Wait)an.

```cpp
#define LED_RED     15    // GPIO number of red LED
// reused by BEE_RST: Green LED not used anymore
//#define LED_GREEN   14    // GPIO number of green LED
```
Die aufgeführte grüne LED hatte ursprünglich die Funktion als weitere Statusanzeige musste aber mangels freier GPIO ports und Stromsparzwecken eingespart werden.

### SPI Devices
Grundsätzlich bietet der ESP32 2 unabhängige SPI ports (VSPI & HSPI) für den Anwender extern an, die per default unter folgenden GPIO Ports (definiert über Arduino.h) erreichbar sind:
```cpp
// ESP32 default SPI ports:
#define VSPI_MISO   MISO  // PIN_NUM_MISO    = 19
#define VSPI_MOSI   MOSI  // PIN_NUM_MOSI    = 23
#define VSPI_SCK    SCK   // PIN_NUM_CLK     = 18
#define VSPI_CS     SS    // PIN_NUM_CS      = 5

#define HSPI_MISO   12    // PIN_NUM_MISO
#define HSPI_MOSI   13    // PIN_NUM_MOSI
#define HSPI_SCK    14    // PIN_NUM_CLK
#define HSPI_CS     15    // PIN_NUM_CS
```
Die Kurzbezeichnungen MISO, MOSI, SCK und SS stammen von der Arduino IDE via Arduino.h und definieren damit den default SPI port, wenn man keine weiteren GPIO Angaben in der Initialisierungsfunktion eines jeden SPI devices macht. (Die tatsächlichen GPIO Werte finden sich im Kommentar)

Zur Einsparung von GPIO Ports habe ich alle 3 verwendeten SPI devices (ePaper, SDCardModul und LoRa Bee) über VSPI angeschlossen.
Während die Leitungen MOSI, MISO und SCk zwischen allen Devices geshared werden (Parallel-Anschluss),
benötigt jedes device mindestens seine eigene CS\ Leitung zum Start der individuellen Protokollfensters.
Um Störung in der Startupphase des setup zu vermeiden werden zu Anfangs in der Setup routine alle CS\ Leitungen der 3 Devices auf inaktiv (High) vordefiniert. Das beugt Störungen bei der weiteren sequentiellen Inbetriebnahme der damit inaktiven SPI Modul Schnittstellen vor.

#### Micro-SDCard Modul
Die Verwendung einer SD Karte ermöglicht einerseits die dauerhafte Ablage großer Mengen an Sensordaten vor dem Versand oder auch als Backup space, wenn keine Konnektivität besteht. 
Anderseits kann im Notfall der komplette Datensatz auch manuell am Laptop ausgelesen werden.

<img src="./images_v2/MicroSDCard_Front.jpg"> <img src="./images_v2/MicroSDCard_Back.jpg">

Dieses weitläufig verfügbare Modul enthält einen 3.3V <-> 5V level changer onboard und ermöglicht dadurch eine Vdd Spannung von 3.3V - 5V mit Logikleitungen auf 3.3V.
Beim Anschluss weiterer SPI Devices stellte sich aber heraus, dass der 3.3V Logiklevel nicht immer sauber eingehalten wird (speziell nach einem Reset). Vdd = 3.3V löste das Problem wieder. 
Dadurch wird allerdings der 5V -> 3.3V Spannungswandler auf dem DevKitC Modul stärker belastet. (Die möglichen 1 A werden wir aber natürlich nicht erreichen.)

Die GPIO Port Definitionen:
```cpp
#include <SPI.h>	// default for all SPI devices
// Libraries for SD card at ESP32:
#include "SD.h"
// ...has support for FAT32 support with long filenames
#include "FS.h"

#define SD_MISO     MISO  // SPI MISO -> VSPI = 19 shared with ePD & LoRa Bee
#define SD_MOSI     MOSI  // SPI MOSI -> VSPI = 23 shared with ePD & LoRa Bee
#define SD_SCK      SCK   // SPI SCLK -> VSPI = 18 shared with ePD & LoRa Bee
#define SD_CS       2     // SD card CS\ line - arbitrary selection !
#define SPISPEED 2000000  //20MHz clock speed
```
Über Nutzung von FS.h ist auch der Betrieb mit long filenames bei größeren Karten möglich.
Dieses Modul unterstützt 2GB - 16GB Micro SD Cards, vorformatiert(!) mit FAT32 Format.

Dieses Modul wäre aber auch ein Kandidat zur EInsparung wenn wir wieter den Strombedarf reduzieren wollen.

#### E-Paper Display
Zur stromsparenden Darstellung der aktuellen Zustands- und Mess-Werte musste ein Display her.
Dadurch sieht man wesentliche Werte und aktuelle massnahmen gleich vorort und nicht nur über eine Webseite remote.

Als Kriterien sollen gelten:
+ Stromsparend auch in der Wait-Loop Phase zwischen den Messungen
+ Gute Ablesbarkeit durch hohen Kontrast auch bei direkter Sonneneinstrahlung
+ Einfacher Anschluss an bestehende ESP32 Interface-Pegel: 3.3V
+ Einfache Ansteuerung und Kontrolle der Darstellung.

Zumindest die ersten 3 Punkte konnte ich durch das ePaper von WaveShare erfüllen:
+ Waveshare 2.7 Inch E-Paper Display HAT Module Kit 264x176 Resolution
+ 3.3v E-ink Electronic Paper Screen with Embedded Controller
+ for ESP32 SPI Interface

Ein Stromverbrauch entsteht nur in der Initialisierungs- und Ladephase der Darstellungsdaten. 
Größter Vorteil ist aber das passive Darstellungsmedium: ePaper, welches auch bei direkter Sonne wie ein gedrucktes Papier erscheint. Das erhöht die Lesbarkeit im Outdoor-Einsatz enorm.
Ein LCD Display müsste hier nachgesteuert werden und muss dazu dauerhaft mit Strom versorgt werden.
Hat allerdings den Vorteil der möglichen Beleuchtung bei Dämmerung/Dunkelheit. 
Allerdings mal ehrlich: wer imkert dann noch ???

Daher hat ein EPaper i.d.R. auch keine Hintergrundbeleuchtung. Dieses Modul weißt neben dem eigentlichen Display dafür noch 4 universelle Schalter zur späteren funktionellen Erweiterung von z.B. verschiedenen Darstellungsebenen auf. Dazu später mehr...

<img src="./images_v2/WaveShareFront.jpg">

Ursprünglich stellt dieses Modul einen RaspberryPi Hat dar und hat daher auch eine 40-pol. Buchsenleiste kompatibel zum RPi. Für unsere Zwecke verwenden wir den parallelen Kabelanschluss.
mit einem separaten SPI Interface über einen 8poligen Stecker incl Kabel:

<img src="./images_v2/WaveShareBack.jpg">

Die Anschlussbelegung des SPI Kabelanschlusses:

|Pin |Farbe  |Function|
|----|-------|--------|
|VCC |rot	 |3.3V/5V |
|GND |schwarz|Ground  |
|DIN |blau	 |SPI MOSI pin|
|CLK |gelb	 |SPI SCK pin|
|CS  |orange |SPI Chip Selection, low active|
|DC  |grün   |Data(=1) / Command(=0) selector|
|RST |weiss  |Reset, low active|
|BUSY|lila   |Busy status output, low active|

Den passenden und umfangreichen DemoCode von WaveShare für RaspberryPi in C++ findet man **[hier](https://www.waveshare.com/wiki/File:2.7inch-e-paper-hat-code.7z)**.

Die GPIO Belegung ist atürlich in den leitungen MISO, MOSI und SCK identisch zu SDCard und LoRa Bee.
neben der eigene CS Leitung gibt es noch einen Reset udn einen BUSY "Draht".
Über BUSY kann man den Upload prozess neuer Display Daten pollen. Deklariert man diese GPIO Leitung im Interrupt mode kann eine asynchrone Bedienung über eine ISR (Int. Service routine) implementiert werden. da wir aber eh '10Min.-10Sekunden' lang nichts besseres zu tun haben, reicht das sequentielle Polling.

```cpp
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
```

Wie oben erwähnt fällt die Bedienung aber etwas aufwändiger aus, denn neben dem SPI API sind dann diverse Font Libs, und ggfs. BitMaps zu laden.
Aktuell verwende ich die Library: https://github.com/ZinggJM/GxEPD
mit dem für mein ePaper device spezifische Extension: GxGDEW027C44.h 
```cpp
// Libs for WaveShare ePaper 2.7 inch r/w/b Pinning GxGDEW027C44
#include <GxEPD.h>
#include <GxGDEW027C44/GxGDEW027C44.h>  // 2.7" b/w/r
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "BitmapWaveShare.h"

#define HAS_RED_COLOR     // as defined in GxGDEW027C44.h: GxEPD_WIDTH, GxEPD_HEIGHT
```

Als weiteres Gimmick unterstützt dieses ePaper von Waveshare auch die Farbe rot. Wie sich bei der Anwendung aber herausstellte, verlängert das die eh schon mit >10 Sek. recht lange Display-Update Zeiten nochmal zusätzlich. **Hier ist eine Optimierung zur Stromeinsparung noch zu implementieren**.
<img src="./images_v2/WaveShareFront.jpg">

Ist da Display einmal aktualisiert, ud die BUSY Leitung lässt uns "weiter arbeiten/schlafen", liegt der Stromverbrauch aber nahezu bei 0mA.

#### LoRa WAN Support
Für die remote Connection ohne "stromfressenden" WiFi Betrieb oder nicht-erreichbarem Hotspot, ist ein LoRa Funktmodul vorgesehen. Auf 868MHz voreingestellt kann es abhängig von der räumlichen Topologie Reichweiten bis zu 8km ermöglichen.

Das LoRa-WAN Protokoll ist auf geringe Band-Belastung und geringem Stromverbrauch ausgelegt.

Das verbaute Funkmodul bietet erst einmal nur den LoRa-MAC layer Übertragungssupport. Den Rest für das **[LoRaWAN Protokoll](https://lora-alliance.org/about-lorawan)** leisten die dazugehörigen Bibliotheken (z.B. die OSS Lib: LMIC von IBM -> search in GitHub).
Darin sind dann eine Peer2Peer Verbindung über unique Sender/Empfänger IDs und Verschlüsselungs-keys ähnlich wie bei TCP/IP kombiniert mit SSH enthalten.
Es zeigten sich bei der Migration der LMIC Lib Instablitäten des LoRa Modes -> es wurden immer wieder FSK Mode IRQs empfangen, was die LoRa Statusführung durcheinander bringt.
Auch ist das von IBM gewählte OS layer Model nicht so handsam wie erwartet.
In Summe stellte die LMIC-Lib für den ESP32 in Kombination mit den übrigen Aktionen zur Sensorbehandlung zumindest auf Node/Cient Seite einen ziemlichen overhead dar, der aber leider nötig ist um das vollständige LoRa-WAN Protokoll nach Spezifikation zu erfüllen (Band-Hopping, Encryption, OTAA joining usw.).
Als Gegenstück ist ein RaspberryPi basierter Gateway vorgesehen, der seinerseits wieder die benötigte WiFi/LAN Anbindung hat, um die gewonnenen Daten zu validieren und auf eine Website zu spiegeln.
Da ein GW ggfs. mit mehreren Clients quasi-gleichzeitig zu tun hat, ist die LMIC STack implementierung optimal. Der OS Layer nimmt einem hier das Queueing hereinkommender Pakete sowie die protokollgerechte Quittierung und Bandmanagement vollständig ab.
Ein weiterer interessanter Client sample Code findet sich über eine Beispielprojekt des Opennet teams:
https://wiki.opennet-initiative.de/wiki/LoRaSensor .

Aktuell befinden sich der Code dazu mangels vollwertigem Gateway noch im Beta-Stadium (!):
Soll heisen auf basis des RADIO layers von LMIC habe ich ein eigenes "schmalspur" WAN Protokoll entworfen (BeeIoT-WAN) welches die Paket Kommunikation auf den rudimentären Austausch von Sensordaten mit einfacher Quittierung (zunächst ohne Multi-Bandmanagement oder Encryption) "optimiert".
Auf dieser Basis habe ich die ESP32 Client- und RaspberryPi GW-seitigen Module entworfen.
Kerneigenschaften der Module:
- Erkennung durch Sender/Empfänger-IDs im "BeeIoT-WAN"-eigenen Header (nur 5 Bytes)
- CRC basierte Datenprüfung
- automatische Quittierung und resend/retry Kommunikation als Flow Control

Was noch fehlt:
- Collison detection (aber ggfs. durch die CRC Prüfung und ResendAnforderung abgedeckt)
- Encryption auf AES Basis (abgeleitet von der LMIC AES Lib)
- Multi Band management (heute wird nur 1 band gewählt, das aber nur alle 10-15 Minuten benutzt)
- Duty Time recognition

Hauptanbieter des LoRa-MAC Layer HW Moduls ist die Firma Semtech, die auch die **[LoRaWAN Spezifikation v1.0.3](https://lora-alliance.org/resource-hub/lorawanr-specification-v103)** als Member der "LoRA Alliance" mit herausgegeben hat.
Die Firma Dragino hat auf Basis dieses Quasi-Standard Modules (basierend auf dem SX1276/SX1278 transceiver chips) diverse Hats & Shields entworfen.
Der kleinste Vertreter davon (ohne GPS Modul) ist das "Dragino Lora-Bee Modul" **[(Wiki)](http://wiki.dragino.com/index.php?title=Lora_BEE)**, welches via SPI angeschlossen wird.

Darauf befindet sich ein RFII95-98W mit SPI Interface. Dieses Basismodul von Semtech kann man aber auch günstig (2-6€) in Asien bestellen) und erfüllen densleben Zweck. Die Draginomodule nehmen einem nur zusätzliche Verdrahtung und ggfs. den Antennenanschluss ab.

Die Verdrahtung ist recht einfach:
Neben den Standard shared (!) SPI Leitungen (MISO, MOSI, SCK) gibt es noch die Modul-spezifische CS Leitung zur Modul-Selektion, eine Reset-Leitung (RST) und 6 universelle Daten-IO Leitungen für weitere Funktionen DIO0..DIO5. 
DIO0 bildet z.B. den LoRa Interrupt ab und triggert bei RX/TX-Events. Für den Standard LoRa Betrieb werden die übrigen DIO1-DIO5 Leitung aber i.d.R. nicht benötigt (solange man beim LoRa-Mode bleibt; Für den FSK Mode werden häufig auch DIO0-2 genutzt).
Daher habe ich in dieser Schaltung nur DIO0 + DIO1 auf duplex fähige GPIO Leitungen mappen können, und DIO2 auf eine Read Only Leitung (weil sie noch frei war, aber geshared mit dem Key3 des ePaper Moduls; aber aktuell ohne Funktion bleibt). Ggfs. kann man darüber noch einen manuellen Sendetrigger imlementieren. 
Alle 3 Leitungen werden aber nur im Input Mode betrieben (zur Signalisierung des Semtech Modul Status).

<img src="./images_v2/Dragino_Lora_Bee.jpg"> <img src="./images_v2/Dragino_Lora_Bee_Cabling.jpg">

Auf dem 2. Bild ist das grün gefärbte Semtech LoRa Modul gut zu erkennen.

Die Spezifikation weisst folgende Eigenschaften aus:
- 168 dB maximum link budget.
- +20 dBm - 100 mW constant RF output vs.
- +14 dBm high efficiency PA.
- Programmable bit rate up to 300 kbps.
- High sensitivity: down to -148 dBm.
- Bullet-proof front end: IIP3 = -12.5 dBm.
- Excellent blocking immunity.
- Low RX current of 10.3 mA, 200 nA register retention.
- Fully integrated synthesizer with a resolution of 61 Hz.
- FSK, GFSK, MSK, GMSK, LoRaTM and OOK modulation.
- Built-in bit synchronizer for clock recovery.
- Preamble detection.
- 127 dB Dynamic Range RSSI.
- Automatic RF Sense and CAD with ultra-fast AFC.
- Packet engine up to 256 bytes with CRC.
- Built-in temperature sensor and low battery indicator.

Das Dragino Manual dazu findet sich **[hier](http://wiki.dragino.com/index.php?title=Lora_BEE)**.

<img src="./images_v2/Duck_Antenna.jpg">
Da aber nahezu alle Leitungen des SemTech Moduls 1.1 am Bee-Sockel ausgeführt sind, kann man im Grunde jede Bibliothek verwenden, die den SX1276 (für 868MHz) unterstützt.

Die aktuell verwendeten GPIO Port Definitionen:
```cpp
#include <SPI.h>	// default for all SPI devices
// Libraries for LoRa
#include "LoRa.h"

// LoRa-Bee Board at VSPI port
#define BEE_MISO VSPI_MISO	// SPI MISO -> VSPI
#define BEE_MOSI VSPI_MOSI	// SPI MOSI -> VSPI
#define BEE_SCK  VSPI_SCK	// SPI SCLK -> VSPI
#define BEE_CS   	12		// NSS == CS
#define BEE_RST		14  	// Reset\
#define BEE_DIO0	33		// Main Lora_Interrupt line
#define BEE_DIO1	13		// for Bee-Events
#define BEE_DIO2	34		// unused by BEE_Lora;  connected to EPD K3 -> but is a RD only GPIO !
```
Die Rolle des Client Node MAC layers ist in dieser **[Backend Specification](https://lora-alliance.org/resource-hub/lorawanr-back-end-interfaces-v10)** festgehalten
Aktuell gehen meine Tests, wie oben bereits erwähnt, in Richtung der LMIC LoRaWAN SW Stack Implementierung von IBM. Der MAC Layer ist durch die Implementierungen in hal.c + Radio.c enthalten.
(siehe GitHub -> "lmic_pi"). Dieses habe ich als Basis für den GW seitigen BeeIot-WAN Aufbau auf Basis RPi verwendet.
Alternative sei auch noch die "RadioHead" Implementierng genannt (s. GitHub).

Für ESP32 MAC Layer Testzwecke habe ich im Sketch aktuell die Lora-Library von Sandeep (GitHub) in Verwendung. Diese ist für eine stabile MAC Layer Kommunikation vollkommen ausreichend. Über eine vollständige WAN Kommunikation kümmere ich mich später...

Da ich ein eigens "BeeIoT-"LoRaWAN Netzwerk aufbauen möchte ist die Rolle dieses ESpP32 Nodes: Activation-by-Personalization (ABP) und als eindeutige statisceh LoRa Devide-ID habe ich daher die ESP32 interne BoardID (basierend auf der WiFi MAC Adresse) vorgesehen. Daraus kann auch die LoRa-"DevAddr" zur Node-Protokoll ID (DevID) gebildet werden.
Eine Anbindung an das allseits beliebte offene TT-Netzwerk schränkt die Nutzung durch limitierte Anzahl Daten und Paket/Zeitraum zu sehr ein.
Als Advanced feature ist OTAA anzusehen. Der standardisierte Weg über LoRaWAn FW updates remote auf den ESP32 als Lora Node aufzuspielen. Damit ist echte Fernwartung möglich, (wie sonst nur bei einer echten LAN Verbindung und laufendem OS auf dem Node wie bei einem RPi.) Für solch intensiven Datenaustausch ist aber Multiband management mit Duty Time Control nötig, weil sonst das einzelne Band zu sehr vereinnahmt wird.

### NarrowBand-IoT
NearBand-IoT ist grundsätzlich eine LTE basierte Kommunikation mit SIM Karte und Provider, wie bei jedem Smart-Mobile auch. NB-IoT verwendet aber zusätzlich die niederfrequenten Band-Anteile und erreicht damit eine bessere Durchdringung von Gebäuden. Ein Client im 3. Stock einer Tiefgarage soll damit problemlos möglich sein, was für die meisten Smart-Home Anwendung aureichend sein sollte.

Als Sender mit einem SIM Kartenleser Modul habe ich mir den häufig verwendeten und Library seitig gut unterstützten SIM700E mit GPS Maus (optional) support ausgesucht.

Leider benötigt es als echtes SIM Modem eine serielle RX/TX ANbindung, wofür weitere 2 GPIO Leitung benötigt werden. Da diese aktuelle nicht frei sind, bleibt es erstmal bei der LoRaWan Anbindung.

Das von mir bestellte Modul: 
Waveshare NB-IoT eMTC Edge GPRS GNSS Hat incl. Antenne
+ mit Breakout UART control pins (Baudrate: 300bps~3686400bps)
+ Control via AT commands (3GPP TS 27.007, 27.005, and SIMCOM enhanced AT Commands)
+ Supports SIM application toolkit: SAT Class 3, GSM 11.14 Release 98, USAT

<img src="./images_v2/SIM7000E_NBIoT.jpg">

=> Link zum **[SIM7000E-HAT Wiki](https://www.waveshare.com/wiki/SIM7000E_NB-IoT_HAT?Amazon)**

Verwendet folgende ESP32 Anschluss pins (falls verfügbar)
+ TXD0: Optional
+ RXD0: Optional

=>In V2.0 aber nicht implementiert !

### RTC Uhrzeit-Modul
Für eine gutes Monitoring und auch für die LoRaWAn Kommunikation ist stets die genaue Uhrzeit zur Synchronisation der Datenpakete nötig.

Für eine genaue Uhrzeit kommen 2 Quellen in Frage:
- Befragung eines NTP Servers via WiFI Anbindung, oder
- via localem RTC Module

Beide sind im Sketch implementiert und redundant zueinander verschaltet:
Besteht keine WiFi Verbindung wird das RTC Modul direkt befragt. Ansonsten wird via WiFi Verbindung Kontakt zu einem NTP Server aufgenommen und das lokale RTC Modul neu mit der NTP Zeit synchronisiert.
Darüber erhalten wir stets eine hinreichend genaue Uhrzeit vor.

<img src="./images_v2/RTC_DS3231.jpg">

Dieses RTC Modul mit DS3231 chip enthält neben einem sehr genauen Uhrzeitmodul auch ein internes EEPROM zur residenten Ablage von Betriebsdaten im Sleep Mode. Dies ist eine zusätzliche Alternative zur ESP NVRAM area oder gar der SDCard. Weitere Test müssen aber noch herausarbeiten, welcher Weg den geringeren Stromverbrauch bei hinreichender Speichergröße für die Sleep Mode Housekeeping Daten darstellt. Auch ist die Häufigkeit der Widerbeschreibbarkeit ein Thema, da der Sleep Mode alle 10-Minuten gestartet wird.

Dieses EEPROM 2432 ist mit einer eigenen Stromversorgung über ein 3V onboard Lithium-Akku (LIR-2032/3.6V)gepuffert.
Alternativ kann der Li-Akku auch durch eine normale 3V Li Zelle (CR2032 oder CR2016) ersetzt werden, dann aber mit endlicher Laufzeit. Eine "LostPower()" Funktion erlaubt einen Batterie-Stromausfall zu erkennnen und nachträglich abzufragen, um dann ggfs. die Uhrzeit neu zu stellen.

In rot umrandet eingezeichnet sind die, von mir vorgenommenen und **[allseits empfohlenen Änderungen](https://thecavepearlproject.org/2014/05/21/using-a-cheap-3-ds3231-rtc-at24c32-eeprom-from-ebay/)** um den Stromverbrauch weiter zu reduzieren:
- Als Massnahme zur Stromreduktion:
	* Abschalten der Power LED durch Abheben des Widerstandes links neben der POWER LED (oben im Bild).
- Alternativer Batteriebetrieb (onboard)
	* Ein Jumper unterbricht den Ladekreislauf für den onboard Li-Akku, wenn stattdessen eine standard Li-Batterie verwendet wird. -> Ablöten des SMD-Wiederstandes 220Ohm und Ersatz durch Jumper und STd. 220Ohm Widerstand in Reihe (Im Bild u. rechts; Widerstand ist auf der Rückseite verbaut).
- Die I2C Adressierung kann im Bild unten links geändert werden: A0..A2.
- Für meine Zwecke habe ich A2 kurzgeschlossen, was folgende Adressierung des RTC Moduls ergibt:
	* RTC_Clock: 0x68
	* RTC_Eprom: 0x53

Das ergibt folgendes Mapping im I2C Adressraum des ESP32 Treibers:
<img src="./images_v2/I2C_AddressScan.jpg">
Hinter der Adresse 0x48 verbirgt sich das ADS1115 Modul.

Das I2C API wird am ESP32 über 2 frei definierte GPIO Leitungen realisiert:
```cpp
// RTC DS3231 Libraries
#include "RTClib.h"	
// based on Wire.h library
// ->referenced to pins_arduino.h:
// static const uint8_t SDA = 21;
// static const uint8_t SCL = 22;

RTC_DS3231 rtc;     // Create RTC Instance
```
Die RTClib unterstützt die RTC Typen:	RTC_DS1307, RTC_DS3231, RTC_PCF8523

Die rtc.begin() Funktion stützt sich auf die default I2C GPIO Einstellungen der Wire Lib ab, wie sie über die IDE im Rahmen der Standard Wire-Library (Wire.h) definiert, verwendet werden.

Über das RTC Modul kann man sehr elegant zu Testzwecken weitere I2C Module anschliessen (unten im Bild), da die I2C Leitungen durchgeschleift wurden. Die benötigten Pullup Widerstände für SDA udn SCL Leitung befinden sich zur Entlastung der ESP32 Ausgangstreiber ebenfalls onboard, sichtbar durch 2 SMD chips mit dem Aufdruck 472 (verbrauchen aber ca. 4 mAh !).

Als weiteres Feature führt dieses RTC Modul einen internen Chip-Temperatursensor, den man elegant auslesen kann. In diesem Fall verwende ich ihn zum Monitoring der Extension BOX internen Temperatur, um einem ev. Hitzetod der Elektronik an heissen Sommertagen vorzubeugen.

Zuletzt gibt es noch einen SQW Pin, an dem man sehr genaue Frequenzen programmieren kann um ext. Prozesse zu steuern. Aktuell wird er in diesem Projekt aber nicht verwendet -> DS3231_OFF ... aber gut zu Wissen.
```cpp
/** DS3231 SQW pin mode settings */
enum Ds3231SqwPinMode {
  DS3231_OFF            = 0x01, // Off
  DS3231_SquareWave1Hz  = 0x00, // 1Hz  square wave
  DS3231_SquareWave1kHz = 0x08, // 1kHz square wave
  DS3231_SquareWave4kHz = 0x10, // 4kHz square wave
  DS3231_SquareWave8kHz = 0x18  // 8kHz square wave
}
```


## Power Management
Als größter und wichtigster Stromverbraucher gilt das ESP32-DevKitC board im Wifi + BT Modus mit bis zu 100mA an 3.3V. Diese Stromversorgung erhält der ESP32 selbst aber über das DevKitC Board und einem onboard Spannungswandler (AMS1117-3.3) von einer 5V USB Buchse.
Über den externen USB Port kann ebenfalls eine Ladespannung von 5V angelegt werden. Diese kann von einem PC, USB Netzteil oder einem 5V Photovoltaik Ladepanel stammen.

So existieren auf dem Extension Board 2 Hauptversorgungsleitungen:
- +5V: von der Batterieversorgung (die widerum auch die Ladespannung beziehen kann)
	+ Verbraucher sind: 
		* 3x OneWire Bus -> Temperatur Sensoren
		* ADS1115 -> Power monitoring
		* RTC Modul -> Uhrzeitversorgung
		* 4-port LevelChanger -> für ADS1115
- 3.3V: erzeugt auf dem ESP32 DevKitC Board. (Der Wandler unterstützt max. 1A Last)
	+ Verbraucher sind:
		* ESP32 Wroom/Wrover-B + DevKitC Board Elemente
		* HX711 + Bosche Weight Cell
		* Micro SDCard Modul
		* LoRa Bee Client
		* ePaper Display
		* Monitor-LED

Manche Module benötigen aber auch 5V, wodurch als Gesamt-Extension BOX Versorgungsspannung 5V gewählt wurde.
Diese 5V können über verschiedene Wege bereitgestellt werden:

<img src="./images_v2/BeeIoT_Client_Powerplan.jpg">

Ohne weitere Optimierungen und ohne Sleepmode verbraucht der akt. Aufbau ca. 200mA im aktiven Zustand.

### Externer USB Port
Der direkteste und einfachste Weg ( wie auch zu testzwecken bei Arduinos üblich) ist natürlich direkt über einen externen USB Port als Ladeanschluss (max 1A möglich).

**Vorteil:** Einfach und günstig über ein USB Ladegerät bereitzustellen.
**Nachteil:** Die Reichweite ist zur Vermeidung von Stör-/Strahlungseinflüssen auf wenige Meter beschränkt.
–> Es erfordert einen wetterfesten Netzanschluss „in der Nähe“ (ev. ideal für die Heimgartenlösung).

In diesem Projekt wird ein exter USB Port als Lade- und Maintenance Port verwendet:
- Die 5V Leitung geht als Charge input an das Batteriemodul.
- Die Datenleitungen gehen aber direkt an den internen USB port des ESP32 devKitC Boards, welches wiederum seine stablisierten 5V vom Ausgang der Batterieversorgung erhält.

Hierfür habe ich steck-/lötbare Micro USB Buchsen verwendet und die zuleitungen wie oben beschrieben gesplittet.
<img src="./images_v2/MicroUSB.jpg">

Hierüber ist damit weiterhin ein problemloser offline Betrieb mit Datenzugriff zum FW upload auf den ESP32 möglich.

### Batterie/Akku
Als ladbare Batterie kämen in Frage ein

+ Gel/Blei Akku
	- Positiv: 	günstig im Einkauf
	- Negativ:	hohes Gewicht und Volumen
+ Lithium-Fe Akku
	- Positiv: 	Hohe Energiedichte -> geringes Volumen und Gewicht
	- Positiv:	lange Haltbarkeit der Ladung
	- Negativ:	sensibel bei niedrigen Temperaturen
	- Negativ:	hoher Preis

Natürlich wäre eine extern geladene Lithium Batterie wie z.B.
	Lithium Akku 12-7.5 12,8V 7,5Ah 96Wh LiFePO4 Lithium-Eisenphosphat für 100€
über einen StepDown Wandler auf 5V der einfachste Weg eine konstante Spannungsversorgug sicherzustellen. Ist von 12V kommen aber mit größeren Wandlungs-Verlusten behaftet, und aufgrund von Gewicht und Abmaßen recht unhandlich.

Eleganter geht es mit einem Akkupack, welches in die Waage eingebaut werden kann und gleich ein Lade/Entlade Management Modu mit sich bringt. Viele dieser Varianten unterstützen aber nur Be- oder Entladung, gekoppelt mit einem weiteren Steckzyklus bei der Umschaltung.

Es gibt aber auch sogenannte **"Passthru"**-Regler die gleichzeitg Laden und Entladen beherrschen.
Ein günstiger Vertreter ist: **[POWERADD Pilot Pro4 Powerbank](https://www.amazon.de/dp/B07FZ27Z77/ref=pe_3044161_185740101_TE_item)** mit den Eigenschaften:
- mit 30.000mAh Kapazität
- 3 USB Output(5V/2,1A)
- 2x USB 2A-Charge Input (parallel)
- mit Lade-/Entladeintelligenz kompatibel zu  iPhone XR/XS/X / 8 / 8Plus / 7 Samsung Galaxy usw.
- Preis: 25,99€

Dummerweise ist die interne Lade/Entladeintelligenz auch hinderlich, wenn sie der Meinung ist, ein ESP32 im Sleep Mode wäre "nicht vorhanden". Denn dann wird der Ausgang einfach abgeschaltet. Genauere Messungen ergaben aber, dass der Ausgang nicht vollständig abgeschaltet wurde, sondern nur der StepUp Regler umgangen wurde. Es lagen dann am Ausgang die direkte Batteriespannung zw. 3.2 - 4.2V an.
Daher habe ich mich entschlossen die Charge Funktion zu erhalten, die Ausgangskontrolle aber einfach zu umgehen und die internen Anschlüsse direkt anzuzapfen:
<img src="./images_v2/BeeIoT_PowerBank.jpg">

- rot -> USB output (mit temp. Abschaltung)
- grün -> Charge Input (+3-5V)
- weiss -> Akku + Anschluss direkt (3.2 - 4.2V)
- schwarz -> Masse / Akku - Anschluss

Dadurch haben wir innerhalb der Extension Box alle Optionen diese Powerbank zu integrieren:
- Die ext. USB +5V Leitung an den Charge input
- Der USB-Out Ausgang wird über einen nun externen StepUp Boost Regler von Sodial (4,35€) geführt
<img src="./images_v2/LinearBooster_LM2577.jpg">

Der Linearregler: LM2577  liefert einstellbar saubere 5V und regelt definiert auf 0V runter, wenn die IN-Spannug auch gen 0V geht (Ein-/Ausschaltverhalten). Überschreitet der EIngang allerdings den eingestellten AUsgangswert von 5V, steigt auch der Ausgang mit an ! In unserem Fall aber ken problem, weil der LiFe Akku kaum mehr als 4.2V liefern könnte.
Weitere Eigenschaften des LM2577:
- Eingangsspannung: DC 3-34V, Eingangsstrom: 3A (max.)
- Ausgangsspannung: DC 4-35V (stufenlos einstellbar), Ausgangsstrom: 2,5 A (max.)

Ein ähnliches Exemplar nur ohne LED ANzeige:
**ANGEEK DC-DC Boost Buck Adjustable Step Up Step Down Automatic Converter XL6009 Module** 6,99€
aber mit einem BuckBoost Converter XL6009, regelte am Ausgang plötzlich auf ca. 15V hoch, wenn die Eingangsspannung unterhalb 3.2V ging. Erst weit unter 2V ging die Ausgangsspannug auch gegen 0V.
Diese Episode hätte den ESP32 in die ewigen Jagdgründe geschossen.
So bleibt es erstmal bei dem etwas mehr Strom verbrauchenden Wandler mit LED Anzeige und Linearregler.

Die so gewonnen 5V werden so lange geliefert, wie der LiFe-Akku nicht unter 3.2V kommt.
Darum habe ich im Programm über einen ADS1115 gemessen folgende Batterieschwellwerte festgelegt, die für jede 3.7V LiFe Akku gelten:
```cpp
#define BATTERY_MAX_LEVEL        4150 // mV
#define BATTERY_MIN_LEVEL        3200 // mV
#define BATTERY_SHUTDOWN_LEVEL   3100 // mV
```

An dieser Stelle hilft es die Energie-Rechnung aus Kapitel:  **[Die MCU Arduino/ESP32 Platform](#die-mcu-arduino-ESP32-platform)** nochmal nachzurechnen:

- Anteil Aktivphase: 10 Sek./10Minuten: 1/60
- auf 6 Monate entspricht das (6x30x24)h/60 = 72h = 3Tage Aktivphase
- => 4320-3 Tage Passivphase = 4317 Tage
- Stromverbrauch Passivphase: 4317 x 1mAh = 4317mAh von nun **30.000mAh** 
- Möglicher **Stromverbrauch Aktivphase**: (30.000mAh - 4317mAh)/72h = 5683mAh / 72h = **~365mA**

Pro Tag kämen wir auf einen Gesamtverbrauchsmix:
- Aktivphase : 24 x 6 x 10Sekunden = 24 Minuten -> 80mAh
- Passivphase: 24 x 60 - 24Minuten = 1416 Minuten -> 59mAh
in Summe also 139mAh/Tag ergibt bei 30.000mAh Akku Kapazität = **215 Tage Laufzeit**.

Und das ohne jede Ladung. Ergänzen wir das ganze noch mit einem 5V PV Modul am ext. USB port ...

### Power Monitoring
Um die oben genannten Lade-/Entladezyklen verfolgen zu können, habe ich einen 4-port AD Wandler ADS1115 spendiert der über einen 3.3V <-> 5V levelchanger ebenfalls am I2C Port des ESP32 hängt.
<img src="./images_v2/ADS1115.jpg">

Falls eingestellte Schwellwerte erreicht werden, wird die Alertleitung als Interrupt genutzt.
Dies ist z.B. der fall wenn der BATTERY_MIN_LEVEL an der Akku+ Line erreicht wird,
dann muss die Stockwaage abgeschaltet werden (bzw. Dauersleepmode) um den Lithium-Akku zu schützen.

Für die eigene Versorgungsspannung Vcc= 5V des Converters habe ich 5V gewählt um ein größeres messbares Spannungsfenster an den AnalogPorts verfügbar zu haben: 0V – 4,096V -> 1mV / Step .
Die 3.3V GPIO Pegel werden über einen duplexfähigen Level Converter per Datenleitung auf 5V Pegel umgesetzt.
Über 2x 3.3kOhm Widerstände werden am ADS1115 AnalogPort 3 ein Spannungsteiler zur Messung der (eigenen) 5V Spannungsversorgung ermöglicht. Ähnlich müsste man bei größeren Spannungsquellen verfahren die >4V liegen.

Durch die Verschaltung des ADDR = 0 Anschlusses erhalten wir die I2C Adresse 0x48. Somit kein Konflikt mit dem RTC Modul zu befürchten.
Hier die Definitionen des ADS im Sketch:
```cpp
#include <Adafruit_ADS1015.h>	// support for ADS1015/1115

// ADS1115 + RTC DS3231 - I2C Port
#define ADS_ALERT   27    // arbitrary selection of ALERT line
#define ADS_SDA     SDA    // def: SDA=21
#define ADS_SCL     SCl    // def. SCL=22
// ADS1115 I2C Port Address
#define ADS_ADDR            0x48   // I2C_ADDRESS	0x48 -> ADDR line => Gnd
// ADS device instance
Adafruit_ADS1115 ads(ADS_ADDR);    // Use this for the 16-bit version
```

Dank der Adafruit Library ist die Nutzung des recht komplizierten aber leistngsfähigen I2C Interfaces des ADC1115 sehr einfach geworden:
```cpp
	adcdata = ads.readADC_SingleEnded(channel);	//channel = 0..3
	// ADC internal Reference: +/- 6.144V 
	// ADS1015: 11bit ADC -> 6144/2048  = 3 mV / bit
	// ADS1115: 15bit ADC -> 6144/32768 = 0.1875 mV / bit
	data = (int16_t)((float)adcdata * 0.1875);	// multiply by 1bit-sample in mV
```

### PV Solar-Modul
Die oben errechneten 215 tage Laufzeit können ggfs. noch verlängert/stabilisiert werden, wenn eine zusätzliche Stromversorgung zur Ladung ins Spiel kommt.
Zur Erhaltung der Mobilität liegt die Lösung in einem externen PV Modul/Panel:

	
<img src="./images_v2/SolarPanel_RAVPower.jpg">
**Solar Charger RAVPower 16W Solar Panel für 46€ (23.9 x 16 x 2 cm)**

Die Besonderheit liegt in der hohen Dynamik der Energieversorgung durch wechselnde Sonneneinstrahlung, weswegen ein nachgeschalteter interner Laderegler via Batterie die Energie puffert und in die gewünschte Zielspannung von 5V umsetzt.
Auch hier ist das Angebots-Spektrum sehr groß, von einem 180W Panel 1,8m x 0,8m Größe bis hin zu einem 40W Faltpanel, teilweise gleich mit Micro USB Kabel Anschlüssen.

Auch hier müssen wir eine Lösung finden, die im Schnitt 5V liefert und dies mit ausreichender Leistung. Solche Module sind häufig im Campingbereich zu finden, als Kompromiss zw. Mobilität und Leistung.

In unserem Fall wäre es kein großes Problem ein kleines Panel neben die Beute zu platzieren.
Im Winter läuft man allerdings Gefahr, dass der Schnee zu lange die Energieversorgung ausbremst und die Batterie leerläuft.

Die Handhabung ist denkbar einfach: Auseinanderfalten, mit einem Nagel an der Beuten-Südseite sicher befsetigen, und per Micro USB Stecker an das Steckerpanel der Stockwaage, wo sich der ext. USB Connector befindet. 
Den Rest erledigt die Akku-interne Ladekontrolle, solange das Panel 5V liefern kann. Und das ist dank eines PV-nternen step Reglers recht lange der Fall.
Bei einem trüben aber freundlichen Märztag war eine PV Modul interne interne PV-Modul-Spannung von 14,6V über mehrere Stunden gegeben. Tests bei Vollsonne stehen noch aus…

Langzeittest besonders bzgl. Wetterfestigkeit stehen aber noch aus.

### Alternativer Batterie Laderegler
Neben dem günstigen obe beschriebenen PV Modul mit integriertem 5V Ausgangsregler via USB Stecker, sind aber noch leistungsfähigere Module denkbar, die aber auch leistungsfähigere Laderegler für z.B 12V Akkus benötigen.

Die effiziente Energieverwaltung/Verteilung durch zeitgleiche Ladezyklen und Verbrauchsphasen soll über ein eigenes Ladekontrollmodul erzielt werden.

Erste Versuche in diese nächst höhere Leistungsklasse habe ich mit dem sehr günstigen **PV Solar Panel MPPT Laderegler von Sunix** gestartet: 

<img src="./images_v2/SolarCharger.jpg">

**Modell: SU-SU702  (CMTD 2420) / 10A mit den Maßen: 14,4 x 8,3 x 4,1 cm für 13,99€**

Er hat die interessante Zusatzfunktion neben jeder Eingangsspannung bis 70V und der Batterieausgangsspannung zw. 12-14.3V auch 2 Standard USB Ports anzubieten mit geregelten 5V.
Diese würden direkt für den Betrieb der ESP32 Stockwaage herhalten können.

Die Kennwerte dieses Moduls sind:
> Nennspannung : 12V / 24V (Auto-Switch)
> Max. Lade / Entlade-Strom : 10A
> Max. Solar-Panel Eingangs-Spannung : =50V
> Stop-Ladespannung : 14.7V / 29.4V
> Nieder-Voltage-Wiederherstellung : 12,2 V / 24.4V
> Nieder- Spannungsschutz : 10.5V / 21.0V
> USB-Ausgangs-Spannung / Strom : 5V 2A
> Kein- Lade-Verlust : =10mA
> Temperatur Kompensation : -3mV / Cell / ° C
> Betriebs-Temperatur : -20 ° C ~ 60 ° C

Mit folgenden Schutzmassnahmen:
1. Überlastschutz
2. Kurzschlussschutz
3. Blitzschutz
4. Unterspannungsschutz
5. Überladeschutz
6. Verpolschutz

Mit diesem Laderegler ist es möglich die Batterie-Beladung durch unterschiedliche Stromquellen über Dioden zusammengeführt zu konfigurieren, während ein Verbraucher parallel versorgt wird (können viel Batteriepacks nicht: nur Laden oder nur Liefern)=> PassThru Mode.

Über 2x BY500 Dioden lassen sich weitere Stromlieferanten am Eingang einpflegen.

Bei dem angedachten Verbrauchspegel des ESP32 Moduls ist dieser Ansatz aber ein Overkill, da alleine der Eigenverbrauch höher liegt, als der, des zu versorgenden Verbrauchers.


## Der Aufbau
Die Anforderungsliste liest sich gut:
+ Das Aussengehäuse besteht aus wasserfesten 10mm MBF Platten sowie einer Kantholz Auflage-Konstruktion
+ Die MCU Extension Box (Compute Node Modul) ist in ein wasserdichtes Gehäuse der Schutzklasse IP67 eingebaut
+ Lade-Kabelanbindung mit Nagetierschutz (Spiralmantelung)
+ Messbereich: 0..100kg mit +/- 60 Gramm Genauigkeit und extrem geringer Temperaturdrift
+ Start der Messung und Übertragung automatisch bei jedem Re-/Start
+ Alle Kabel werden an das Gerät durch wasserdichte Kabelverschraubungen angeschlossen.
+ Alle am backpanel angeschlossenen Sensoren sind auch wasserdicht, Schutzklasse IP-66 oder IP-67.
+ Der Gehäuseaufbau erlaubt das einfache Anpassen der Auflagefläche durch massgerechte Unterlegplatten für jede Beutengröße verwendet werden (default: Zander-Maß).


### Stückliste

| Index | Stück | Bezeichnung | Hersteller | Bezugsquelle | Preis | Kommentar |
|-------|-------|-------------|------------|--------------|-------|-----------|
|1|1|Plattformwägezelle H40A-C3-0100|Bosche|http://www.bosche.eu|58€|H30A v H40A 100kg|
|2a|1|ESP32-DevKitC v4|OSS|Amazon|20€|(WROOMER32 oder WOVER32)|
|2b|1x|SD HC Karte 2GB|||Amazon|5€|
|3|1x|HX711 24 Bit A/D Wandler||Amazon|10€|mit grüner Lackschicht!|
|4|2x|Montageplatte für Wägezelle ALU 200 x 200 x 10mm|Schlosserei||200€|Keine Beschichtung|
|5|2x|4-kant Unterlageholz|Baumarkt||10€|20x40x500mm|
|6|1x|IP67 Kunststoff-Montagebox|Maße 70x80x160mm||Amazon||
|7|3x|Temperatursensor DS18S20|Dallas|Amazon|7,5€|Mit 1,1m Kabel|
|8|1x|MSP Siebholzplatten 520x410x10mm |Baumarkt|Baumarkt|20€||
|9|1x|MSP Siebholzplatten 490x380x10mm |Baumarkt|Baumarkt|20€||
|11|2x| 520x70x10mm|Baumarkt|Baumarkt||5€|
|12|2x| 390x70x10mm|Baumarkt|Baumarkt||5€|
|13|16x|M12 30mm Senkkopfschrauben|Baumarkt|Baumarkt|16€||
|14|18x|4x50mm Senkkopfschrauben|Baumarkt|Baumarkt|10€|Verbindung der MSP Platten+Leim|
|15|1x|POE Einspeiser & Wandler 1Gb/s & POE 48V->5V||Amazon|15€|1Gb/s & Mini-USB|
|16|div.|Litze D:1mm ca. 3m||Amazon/Conrads|10€|Sensorverdrahtung|
|17|1x|Alu/Kunststoff-Platte 120x60x1,5mm||Amazon|8€|Anschlusspanel|
|18|1|5-pol wetterfeste Durchführungs Rundstecker & Buchse||Amazon|5€|Für ext. OneWire Sensoren|
|19|1|Epoxid Rundlochplatine Euro-Format(einseitig)||Conrads/Amazon|5€|ESP32 + GPIO Sensorverstärker (HX711 und OneWire Bus)|
|20|1|Sub-D 25-pol. Stecker & Buchse||Conrads/Amazon|5€|Zum Durchführen der HX711 und OneWire Bus-signale am K.Stoff-Gehäuse|
|21|1x|4-channel Level Converter für 3V <-> 5V Pegel,  bidirektional||Amazon|4€||
|22|1x|A/D Converter ADS1115S||Voelkner|5€|4-fach 16Bit A/D Wandler mit I2C API zur Versorgungs-Spannungsmessung|
|23|1x|Micro USB Buchse||Amazon|8,50€|Zur USB-A Port Durchführung am Aussenpanel|
|24|2x|10-polige Steckerleiste|Printmontage|Amazon|1,50€|Stecker für die ADS1115S + HX711 Ports|
|25|2x|8-polige Steckerleiste Printmontage||Amazon|1,50€|Stecker für das ePaper 2in7 API + OW/LED Leitungen|
|26|2x|3.3kOhm Metallfilm Widerstand 1/4W||Voelkner|0,10€|Als Spannungsteiler am Analogport 3 des ADS1115S|
||Optional:||||||
|27|1x|ePaper Display 2in7 von WaveShare 264x176 pixel 3.3V||Amazon|25€|Extrem stromarmes S/W Display 2,7 Zoll mit SPI Interface|
|28|1|RJ45 1Gb/s wetterfeste Durchführungsbuchse||Amazon|10€|Für LAN & POE Anschluss|
|29|1|LAN Kabel Cat6 Outdoor 10-30m||Amazon|20-40€|Länge nach Erreichbarkeit des Switch|


### Das Aussengehäuse

Das Aussengehäuse bestehend aus MBF Platten besteht aus einer Coverhülle und einer Bodenplatte.
Auf die Bodenplatte ist die Waagrzellenkonstruktion verschraubt. Die Coverhülle, welche von oben mit dem Wäägemodul verschraubt ist, schwebt quasi frei darüber und ermöglicht so die Gewichtskraft auf die Wäägezelle zu übertragen.
<img src="./images_v2/BoxOutsideView.jpg">

Hier sieht man schematisch im Querschnitt die Auflagekonstruktion der Wägezelle.

<img src="./images_v2/BoxSideView.jpg">


Der damit verbundene Deckel + Compute Node + Power Module schwebt über dem Bodenelement und liegt samt komplettem Beutengewicht ausschliesslich auf den etwa 16cm2 Fläche der verschraubten Wägezelle auf. Dies erreicht man über 6-8mm starke Zwischenbleche jeweils an der oberen und unteren Verschraubung der Wägezelle, die je mit 4x M10 Senkkopf-Schrauben fixiert ist.

Steht die i.d.R. 50-70kg schwere Beute auf der Waage schwankt sie recht steif über der Bodenplatte. Dies ist ein Zeichen dafür, dass das Waagzellen Element aktiv die Kraft aufnimmt und nirgends aufliegt.

In der Rückwand ist das Anschlusspanel + ePaper Display Panel eingearbeitet und sowie die notwendigen Aussenanschlüsse.

#### Die Bosche Wägezelle
(Einbau als Single-point Wägezelle)
Die Wägezellen von Bosche für Plattformwaagen gibt es in 4- oder 6-pin Ausführung.
Beide sind funktionell gleichwertig:

<img src="./images_v2/BoscheCell4.jpg"> <img src="./images_v2/BoscheCell6.jpg">

Hier kommt die übliche Wheatstone-Brücke zum Einsatz, die die beste Temperatur-Kompensation bei hoher Messgenauigkeit bietet. Die Temperaturkalibrierung an den Dehnmesstreifen wurde werksseitig schon vorgenommen. Ggfs. bei Temperaturen kleiner -5Grad kann noch eine rechnerische Kompensation softwareseitig überlagert werden und sinnvoll sein.

Der Einbau gestaltet sich sehr einfach. Einzig die Fixierung der jeweils 4 Schrauben pro Auflagefläche muss sehr stabil und genau erfolgen. Hier setzt der Hebel des gesamten Beutengewichts samt Waagedeckel an.

Dazu die Auszugsbilder aus der mechanischen Applikations-Beschreibung:

<img src="./images_v2/BoscheApplication.jpg">

sowie die einfache Konstruktion, wie sie hier zum Einsatz kommt:

<img src="./images_v2/WeightCell.jpg">

Die obere und untere Platte zur Kraftübertragung auf die Wägezelle sollte mind. 6-8mm betragen und idealerweise aus Edelstahl. Aus Kostengründen geht aber auch Aluminium. Dann aber besser mit 10mm Stärke. Die Plattenmaße betragen bei mir 18 x 18cm (darf aber auch größer sein).

Die Distanzscheiben habe ich mit je 4mm gewählt und bestehen aus separaten kleinen Aluscheiben mit dem Maßen 6x6cm inkl. den benötigten 4 Bohrungen für die M12 Schrauben. Ohne die Abstandsscheiben kann die Wägezelle durch das Gewicht nicht verformt werden !
Die Bohrungen in der Platte sollten 1mm größer ausfallen als die Schraubenstärke und nach „aussen“ konisch durch einen Phasenschneider erweitert werden. Dadurch kann man die als Senkschrauben ausgelegten 2x4 M6-M8 Schrauben bündig abschliessen. Dies ist nötig um die Gehäuse-Deckel/Boden bündig auf die Platten zur bessern Kraftübertragung anzubringen.

Die Überlastsicherung besteht aus einer M6 - M8 Gewindebohrung an der untern (oder oberen Platte) und einer Schraube, über die der Arbeitsbereich der Wägezelle eingestellt werden kann. Bei einer 100kg Wäge-Zelle erzeugt man ein Testgewicht von 110kg und man dreht die Schraube solange ein bis sie leicht an der Wägezelle aufstehen. Dadurch wird einer zerstörenden Verformung des Zellenkörpers vorgebeugt (z.B. wenn man sich auf der Beute abstützt bei aufliegendem vollen Honigraum).
An einer Seite schaut das 4/6 adrige Kabel heraus, welches zum A/D Wandler HX711 der Compute-Einheit führt.

Zuletzt wird die Platte durch weitere Gewindebohrungen mit kurzen M12 Schrauben mit dem Gehäusedeckel-/Boden verschraubt.

Hier nochmal das Lagebild der Wägezelleneinheit mittig zum Gehäusedeckel:

<img src="./images_v2/Box_WeightCell.jpg">

#### Das Anschluss Panel
Das Gehäuse Panel ist der einzige Anschlussbereich nach Aussen. Daher werden dort wasserfeste Stecker-Kombinationen eingesetzt:

+ Micro-USB Buchsen-Anschluss zur Akku ladung + ESP32 FW-Upload
+ 5 pol. externer OneWire Sensor Connector für den Bienenstock Temp.Sensor
+ LEDs für Monitor & Status
+ Optional: LAN Anschluss (1GB/s mit POE 48V)
+ Power Switch: EIn finaler Power Switch , wollte man die Stockwaage einige Zeit ungenutzt einlagern. Ansonsten bleibt die Akku-Regelung dauerhaft in Betrieb und leert die Battery.
+ ...Platz für weitere Optionen...

Als Träger dient eine korrosionsbeständige eloxierte ALU Platte (1mm). An dieser lassen sich die Buchsenlöcher gezielt herausarbeiten; sie trägt aber nicht so stark auf wie eine Holzplatte und kann daher in einen Rahmen mit ausgefrästem Grat eingesetzt werden.

Es sind 2 LEDs eingebaut, die Betriebszustände der Stromversorgung und die Betriebsaktivität anzigen können. mangels freier GPIO leitungen ist aktuell nur die rote LED angeschlossen.

Die Aussenansicht des Backpanels + ePaper Panel:
<img src="./images_v2/BackConnectors.jpg">

Die Innenansichten der Anschlussstecker und der MCU Extension-Box:
<img src="./images_v2/BoxInsideOvw.jpg">

##### One Wire Sensoren
One-Wire Sensoren werden über 3 pol. Anschlüsse (Masse-GND, Versorgungsspannung Vcc und Daten) parallel miteinander verbunden.

Durch das OneWire Bus-Protokoll können somit leicht mehrere Sensoren unterschiedlichster Art/Funktion hintereinander, an demselben Anschluss verbunden werden. Bei den ESP32 GPIO Ports sind allerdings nur max. 8 Sensoren in Verbindung mit 3.3V Vcc in Kombination mit abgeschirmtem Telefonleitungen zu empfehlen.
Jeder Sensor hat dabei zur Erkennung eine einzigartige 64 Bit unique ID. Verwendbar sind die Sensoren an Spannungen mit 3-5V. Wir verwenden hier 5V.

Der ESP32 erlaubt mit seinem GPIO Matrix Konzept jeden beliebigen IO-Port zum OneWire Bus-Port zu erklären. Der OneWire Driver wendet das OW-Protokoll, wie vom Hersteller „Dallas“ spezifiziert, dann auf diesen Port an.
Die Störanfälligkeit der Messwerte ist gering, dank digitaler Übertragung. Die Datenleitung benötigt dazu allerdings einen Pullup von 4.7k Ohm einmalig (!) von der Daten- auf die 3.3V Leitung (unabhängig von der gewählten Versorgungs-Spannung!).

Neben den 2 internen Temperatursensoren habe ich über einen 7-poligen Stecker die OW-Busleitungen nach Außen zugänglich gemacht, um weitere externe Sensoren zu ermöglichen.

##### Der DS18B20 OW-Temperatur-Sensor
Der DS18B20 OneWire Sensor ist besonders praktisch für das Messen von Temperaturen in Wasser oder feuchten Umgebungen dank wasserdichtem 1,10m langem Kabel und der vergossenen 3 cm langen Metall-Messsonde.
Der Temperaturbereich reicht von -55 bis 100 Grad.

Dieser OneWire-Digital-Temperatursensor ist sehr präzise (±0,5°C Genauigkeit von -10°C bis +85°C) dank einem internem vorkalibriertem 12Bit A/D Wandler und ist somit für unsere Messungen mehr als hinreichend.
Das wären 4096 Messteilwerte über den gesamten gemessenen Temperaturbereich.

**Verfügbare Ausführungen:**
+ DS18B20 Sensor im Edelstahl-Körper, 6mm Durchmesser, 30mm lang 
+ Kabellänge von etwa 90-110cm lang mit Durchmesser 4mm, unkonfektioniert

	**Sensor-Anschluss mit 4-adrigem Kabel:**
1. Rot:	 	3-5 V Anschluss, 
2. Schwarz:	Masse 
3. Weiß:	1-Wire serielles Datenprotokoll 
4. Die äußere Kupferader wird an die Drahtabschirmung mit dem Stecker/Gehäuse verlötet.

	**Sensor-Anschluss mit 3-adrigem Kabel: **
1. Rot:		3-5 V Spannung 
2. Blau / Schwarz: wird mit Masse verbunden 
3. Gelb / Weiß:	1-Wire Datenleitung

Weitere Links:
* [Dallas Temperature Control Library](http://www.milesburton.com/?title=Dallas_Temperature_Control_Library "Dallas TempControl Lib")
* [OneWire Library](http://www.pjrc.com/teensy/td_libs_OneWire.html "OneWire Lib")

Obwohl die Versorgungsspannung für 1-Wire-Devices normalerweise 5 V beträgt, ist beim ESP32 die verringerte Spannung von 3,3 V nötig, weil dessen GPIO-Ports nur 3,3 V vertragen und durch höhere Spannungen zerstört werden.
Nachfolgende Bilder zeigen die verschiedenen Anschlussmöglichkeiten. Dabei sind die dargestellten Chipformen in unserem Fall in einer Metallhülse vergossen.

<img src="./images_v2/DS1820pins.jpg"> <img src="./images_v2/DS1820pins2.jpg">

(Wie erwähnt: der Pullup Widerstand wird nur einmal intern angeschlossen.)

### Compute Node Box
Das Herzstück des Computnodes ist im BeeIoT-Client Modell der **ESP32-DevKitC**.

Dieser sowie alle weiteren empfindlichen elektronischen Module wurden in eine IP67 dichte Box eingebaut. Das beugt Kondenswasserschäden vor. Widererwartens habe ich auch bei höchsten Aussentemperaturen kein Temperaturproblem am ESP32 gehabt. Dies kommt wohl durch die Isolationswirkung der massiven aufstehenden Beute von Oben:

<img src="./images_v2/ESP32_Controlbox.jpg">

Die MCU sowie alle Sensormodule sind auf einer Lochrasterplatine untergebracht:
Der HX711 A/D Wandler für das Wägezellenmodul, dem Level Converter & ADS1115S, sowie diverser Widerstände und Kondensatoren zur Leitungspufferung. (Details siehe Schaltplan).
Hier der vollständige Ausbau inkl. aller Module (auch dem LoRa Bee in rot):

<img src="./images_v2/BeeIoT_BoardLayout2.jpg">

Über den 25 pol. Sub_D Stecker (links) wird u.a. auch die Akku-Out Zuleitung in die Box geführt, wo sie von dem StepUp Regler (Mit LED Anzeige) auf 5V gewandelt wird.
Von rechts oben führt ein Mini-USB Stecker die externe USB Verbindung in die Box mit dem datenanteil direkt an den ESP32. Von der Lochraster-Platine wiederum führen mehrere Steckervarianten die Sensorleitungen zu dem extern 25-pol. Sub-D Stecker. Hierfür gibt es sicher noch elegantere Steckverbinder Lösungen ev. incl. Abschirmung. Dieser Aufbau erwies sich aber als erstaunlich wenig störanfällig, trotz Ausseneinsatz und einer langen (3m) USB Zuleitung für Testzwecke.

### Steckverbinder
Hier nun eine Übersicht aller verwendeten Steckervarianten:

+ Links:	Der runde 6-polige One-Wire Extension Stecker mit dem typ. 3 OW Bus Leitungen.
+ Mittig:	Der ePaper Stecker direkt an der Panel Rückseite
+ Rechts:	Der 25-pol. Sub-D Stecker als Ausgang von der IP67 Computenode-Box

<img src="./images_v2/BeeIoT_ExternalConnectors.jpg">

Hier nun die Steckerbelegungen aller internen Steckverbinder auf der Lochrasterplatine
<img src="./images_v2/BeeIoT_InternalConnectors.jpg">

Die Adapterplatine mit Komponentenbeschriftung:

<img src="./images_v2/MCU_ExtBoard.jpg">


## Die ESP32 BeeIoT Sketch Software
Anders als bei meiner letzten **[BeeLog Projekt](https://github.com/mchresse/BeeLog)** Beschreibung mit Raspberry Pi gibt es bei ESP32 nicht viel vorzubereiten, da wir keine OS Instanz zu konfigurieren haben.
Eine schrittweise Einführung und Konfiguration der ESP32 Einheit ab der SD Karten Vorbereitung bis zum prinzipiellen Ablauf der Logger SW ist aber ev. doch sinnvoll.

### ESP32 Vorbereitungen
Wer sich noch nicht so sehr mit dem ESP32 auskennt, dem sei diese Einsteiger Buch **[Kolbans Bok on ESP32 v2018](https://www.robolinkmarket.com/datasheet/kolban-ESP32.pdf)** empfohlen.

Die Vorzüge des ESP32 DevKit C habe ich eingangs dieser Beschreibung schon diskutiert.
Hier nun der vollständige Schaltplan der Konstruktion inkl. GPIO Ports am DevkitC board Connector J8 :

<img src="./images_v2/BeeIoT_Schematics.jpg">

Der Testaufbau erfolgte mit einer Lochrasterplatine + Fädeldraht-Technik.
<img src="./images_v2/MCU_Board_Back.jpg">

Die interne WiFi Antenne des devKitC Boards hatte innerhalb meines Hauses keine Reichweitenprobleme.
Auch für das loRaan Modul wird eine Duck Antenne direkt an die Extension Box geschraubt und bleibt damit noch innerhalb der Waagenbox. Sollten sich hier reichweitenproblme anmelden, ist es leicht möglich eine Antenne mit längerem Kabel auserhalb zu platzieren.

Da der ESP32 keine USB Port Erweiterungen ermöglicht bleibt der ext. USB Stecker rein für die Ladespannung oder FW Update reserviert.

Hier die vollständige DevKitC Sockel-Belegung J8, wie sie für alle Modelle ab v4 gültig ist:

<img src="./images_v2/ESP32_DevKit_Pinout.jpg">

Jeder GPIO Port darf allerdings nur mit **max. 50mA an 3.3V** belastet werden.

Nach dem vollständigen Aufbau und Fertigstellung der Verdrahtung des Extension Boards samt externer Sensor Zuleitungen bleibt nur noch eine 2-16GB große Micro SD Karte mit FAT32 zu formatieren (Quick Format reicht) und in das MircoSD Kartenmodul zu stecken. Mein Sketch akzeptiert aber auch eine fehlende SD Karte, und setzt die Messung fort.

**Nun kommen wir endlich zur Software Seite:**

### ESP32 IDE
Bei Ardunio MCU kompatiblen IDEs fällt einem als erstes das Arduine-IDF ein. Sie bietet das nötigste um einen Sketch auf das Modul zu bringen, aber verwöhnt durch VS oder NetBeans sucht man zur Unterstützung eines effizienten Test/Debugging/Release Cycles aber schnell nach Alternativen.

Für meine ESP32 Projekte verwende ich aber **PlatformIO**, wo das Library lifecycle management einfacher zu handhaben ist und vom look-and-feel näher an Netbeans ist, welches ich wiederum für die typischen Cross-compilation/debugging sessions mit einem RaspberryPi verwende.
Die Standard Online Doku findet sich hier: **https://docs.platformio.org/en/latest/what-is-platformio.html**

Zur weiteren Beschreibung der Arduino-IDF sei aber noch folgender Link empfohlen:
**[Windows instructions – ESP32 Board in Arduino IDE](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions)**

oder den **[ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html)**

Vielleicht findt sich ja in nächster Zeit noch eine brauchbare Netbeans Implementierug....

#### PlatformIO
Ich verwende statt der Espressif IDF direkt, die darauf aufbauende PlatformIO als Plugin der Microsoft VSCode Umgebung.
Netbeans wäre noch eleganter, konnte aber bisher keine verlässliche Plugin Anweisung finden, die auch die UART basierte cross compilation unterstützt.

Eine Beschreibung der Erstinstallation von VSCode und der Konfiguration des PlatformIO Plugins findet sich hier: **[PlatformIO IDE for VSCode
](https://docs.platformio.org/en/latest/ide/vscode.html#ide-vscode)**
Jede Menge weiterer Tutorials finden sich mit den Suchbegriffen "platformio tutorial".
Daher gehe ich im weiteren davon aus, dass die Nutzung von PlatformIO gegeben ist, wobei der Sketch natürlich universell verwendet werden kann, wenn man die IDE library links entsprechend anpasst.

**PlatformIO.ini** ist die zentrale Build Steuerdatei von PlatformIO, wo alle compile und link settings per Projekt geführt werden.
Die Definition für das ESP32 DevKitC (esp32dev) in PlatformIO.ini lauten somit:
```cpp
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

; build_flags = 
build_flags = -DCONFIG_WIFI_SSID=\"MYAP\" -DCONFIG_WIFI_PASSWORD=\"MYPASS\"

; Serial Monitor Options
monitor_speed = 115200
monitor_flags =
    --encoding
    hexlify
```


### ESP32 Sensor Libraries + Build

Nachdem die IDE eurer Wahl installierte wurde, solltet ihr ein neues Projekt auf Basis der Angaben, wie oben in der platformio.ini vorgesehen, anlegen.
In den Projektfolder kann das BeeIoT Projekt von Github ge-cloned werden. Ggfs. müssen die Sourcefiles im Projekt der IDE angemeldet werden. Die generierte platformIO.ini Datei der IDE sollte um die Angaben aus der platformIO.tpl Datei angepasst werden.

Es empfiehlt sich die Libraries nach dem herunterladen (z.B. von github, oder unterstützt durch die IDE) per 'lib_deps' Anweisung zu spezifizieren, da es sonst bei gleichen Header File Namen zu falschen Zuordnungen kommen kann (z.B. bei Lora.h).

Diese kann man in platformIO.ini per Lib-Namen oder per Lib-ID angeben (eindeutiger):
```cpp
; Library options
lib_deps =
    GxEPD       		; #2951 by Jean-Marc Zingg
    DallasTemperature   ; #54 by Miles Burton
    RTClib      		; # 83 by Adafruit
    U8g2        		; 942 by oliver
    Adafruit GFX Library; #13
    SdFat       		; #322 by Bill Greiman
    HX711       		; #1100 by bodge
    OneWire     		; #1 by Paul Stoffregen
    Adafruit ADS1X15 	; #342
    LoRa        		; #1167 by Sandeep Mistry

; Options for further Lora tests
;	LMIC-Arduino 		; #852 by IBM
;	RadioHead 	 		; #124 by Mike McCauley
;	Heltec ESP32 Dev-Boards ; #6051 by Heltec Automation
```
Diese Lbraries sollten nach einer Installation z.B. über PlatformIO unter folgendem Pfad mit 'library Name'_ID aufgeführt sein: C:\Users\"username" \'.platformio'\lib

Zuletzt müssen eure WLAN credentials in der build_flags Zeile angepasst werden.

Jetzt müsste der Build durchlaufen und ein firmware.elf File liefern der per upload auf den ESP32 geladen wird.

Der Dependency Graph könnte wie folgt aussehen:
```
Dependency Graph
|-- <GxEPD> 3.0.9
|   |-- <Adafruit GFX Library> 1.6.1
|   |   |-- <SPI> 1.0
|   |-- <SPI> 1.0
|-- <DallasTemperature> 3.8.0
|   |-- <OneWire> 2.3.5
|-- <RTClib> 1.3.3
|   |-- <Wire> 1.0.1
|-- <U8g2> 2.27.2
|   |-- <SPI> 1.0
|   |-- <Wire> 1.0.1
|-- <Adafruit GFX Library> 1.6.1
|   |-- <SPI> 1.0
|-- <SdFat> 1.1.0
|   |-- <SPI> 1.0
|-- <HX711> 0.7.1
|-- <LoRa> 0.7.0
|   |-- <SPI> 1.0
|-- <OneWire> 2.3.5
|-- <Adafruit ADS1X15> 1.0.1
|   |-- <Wire> 1.0.1
|-- <FS> 1.0
|-- <SD(esp32)> 1.0.5
|   |-- <FS> 1.0
|   |-- <SPI> 1.0
|-- <SPI> 1.0
|-- <WiFi> 1.0
|-- <Wire> 1.0.1
|-- <Preferences> 1.0
|-- <ESPmDNS> 1.0
|   |-- <WiFi> 1.0
|-- <NTPClient> 3.1.0
|-- <WebServer> 1.0
|   |-- <WiFi> 1.0
|   |-- <FS> 1.0
```

### Die Programm Struktur
Um sich in den Source schneller zurecht zu finden, hier in paar Übersichten:

Die Client HW Modul Struktur in SW Module übertragen:
<img src="./images_v2/SWModuleSheet.jpg">

Die dazugehörige Klassen-Struktur:
<img src="./images_v2/SWClassView.jpg">

Zuletzt der Funktions-Programmflussplan der Loop() Schleife:
<img src="./images_v2/SWMainFlow.jpg">


#### Setup Phase
Mögliche Debugmeldungen habe ich über ein simples Macro parametrisiert:
```cpp
#define	BHLOG(m)	if(lflags & m)	// macro for Log evaluation
uint16_t	lflags;      // BeeIoT log flag field; for flags enter beeiot.h

void setup(){
// lflags = LOGBH + LOGOW + LOGHX + LOGLAN + LOGEPD + LOGSD + LOGADS + LOGSPI + LOGLORA;
lflags = LOGBH + LOGLORA + LOGLAN;
```
Darüber kann man nach Belieben verschiedene Funktionsbereiche in den verbose mode schalten und analysieren, so dass der Log-Output nicht von unnötigen Meldungen überschwemmt wird.

Mit den obigen 3 Schalter: LOGBH + LOGLORA + LOGLAN
könnte der Konsol-Output wie folgt aussehen:
```
>*******************************<
> BeeIoT - BeeHive Weight Scale <
>       by R.Esser 10/2019      <
>*******************************<
LogLevel: 273
Main: Start Sensor Setup ...
  Setup: ESP32 DevKitC Chip ID = DK8AD5386624
  Setup: Init runtime config settings
  Setup: Init RTC Module DS3231
  RTC: Temperature: 25.00 °C, SqarePin switched off
  RTC: STart RTC Test Output ...
2019/12/13 (Friday) 15:15:52
 since midnight 1/1/1970 = 1576250152s = 18243d
 now + 7d + 30s: 2019/12/21 3:45:58

  Setup: SPI Devices ...
  Setup: HX711 Weight Cell
  Setup: ADS11x5
  Setup: Wifi in Station Mode
  Setup: WIFI port in station mode
  Wifi: Scan started...done
  Wifi: networks found: 3
        1: MyNet (-45)*
        2: YOURNet (-45)*
        3: WLAN-Foreign (-91)*
  WIFI: Connect.ed with MyNet - IP: 192.168.10.99
  WIFI: MDNS-Responder gestartet.
  Setup: Init NTP Client
Friday, December 13 2019 15:15:56
  Setup: Get new Date & Time:
  NTP2RTC: set new RTC Time: 2019-12-13T15:15:56
  RTC: Get RTC Time: 2019-12-13T15:15:56 - 2019-12-13 - 15:15:56
  NTP: BHDB updated by RTC time
  Setup: SD Card
  Setup: LoRa SPI device & Base layer
    Setup: Start Lora SPI Init
    LoRa: init succeeded.
  Setup: ePaper + show start frame
  Setup: OneWire Bus setup
  Setup: Setup done
```
Im Kern durchläuft man in der Setup Phase alle Setup Routinen der einzelnen Komponenten in der Reihenfolge, wi si voneinander abhängig sind und wo möglich nach protokoll Klassen sortiert.
So liegen die SPI devices und I2C devices je intereinander, weil sie teilweise diesselbe Vorbereitung benötigen.
Optional resourcen (WiFi + NTP alsauch das ePaper enthalten einen Error bypass, so dass der Logger auch ohne deren Initialisierung anläuft. Das ist bei einem Stromausfall wichtig, da nach Widereinschaltung das Programm ja munter darauflos-startet und das Logging samt Übetragung fortsetzen möchte.

Die Logik der Setup-Bereiche:
1. Initiaisierung des Log Flag fields
2. Preset des Monitoring LED IO ports auf Output mode => sonst sieht man nichts...
3. Aktivierung des Seriellen Ports für Debugmeldungen
4. Reentry detection aus dem Sleep Mode oder nach Stromausfall (not supported yet)
	a. -> Widerherstellung der Runtime Parameter aus dem residenten Bereich (RTC-EEPROM, oder ESP32-preferences-Area)
	b. Check Grund des WakeUp Calls -> ggfs. Massnahme7Aktion starten (not supported yet)
5. Hole interne unique BoardID aus der ESP32 Fuse-Area
6. Init_Cfg() Initiaisierung aller Strukturen zur Sensordatenbehandlung (interne Mini-DB: bhdb)
7. Init RTC Module
8. Init SPI port
9. Init HX711 ADC for Weight cell access
10. Init ADS1115 for Battery monitoring
11. Discover WiFi and NTP client service
	a. Wenn NTP verfügbar -> read new time => init RTC Modul
	b. If no NTP: Read time from RTC Module only
	c. Start Web-HTTP service to offer config page for firsttime init settings -> wait till done 
12. Init SD Card access -> create new logfile if not detected
13. Init Lora Module for sending -> join to gateway (if any in range)
14. Init ePaper Moduel -> Show startframe of BeeIoT
15. init OneWire Bus -> detect expected 3 temperatur sensors

Seltsamerweise musste ich die Initialisierung des OneWireBus APIs an das Ende setzen, sonst werden die Temperatursensoren nur beim ersten mal richtig ausgelesen, und danach immer mit denselben Werten.
ToDo: => Ein Fehler im Programm oder in der Library ?

Die Aktionen über WiFi ( Scan + NTP + WebPage) würde ich im Normalbetrieb bei funktionierendem LoRaWAN wahrscheinlich ganz abschalten müssen, um den erwarteten Stromverbauch zu erreichen, denn in der Pampa habe ich keinen WiFi-AP nötig. Für den Heimbetrieb ist es wiederum  effektiver als LoRaWan.

>Hinweis:
>Es kommt vor, dass nach dem Upload das Programm schon erwartungsgemäß losläuft und der Welcome Screen gezeigt wird. In der Regel wir das WiFi auch konnektiert. Starten man in der Setupphase (also bis zum Welcome Screen) den Serial Monitor der IDE, wird das Programm "mitten drin" neugestartet, aber eine WiFi Connection schlägt dann meist fehl. In dem Fall ist ein Stop (^C) und Restart des seriellen Monitors nötig um auch den WiFi Betrieb zu erhalten. Möglicherweise nimmt der WiFi Router zeitlich zu eng liegende Reconnect Anforderungen übel...

### Loop Phase
Wie bei Arduino's üblich, wird nach dem Ende der Setupphase automatisch die Loop Routine angesprungen; diese dann aber in einer Endlosschleife:

Am Serial Monitor zeigt sie sich so:
```
>>****************************************<
> Main: Start BeeIoT Weight Scale
> Loop: 0  (Laps: 0)
>****************************************<
  RTC: Get RTC Time: 2019-12-13T15:16:12 - 2019-12-13 - 15:16:12
  NTP: BHDB updated by RTC time
  MAIN: 0,2019-12-13,15:16:12,2.03,21.19,21.12,20.88,25.00,3.28,5.08,4.52,3.67,49
    LoRa[0x77]: Sent #0:<0,2019-12-13,15:16:12,2.03,21.19,21.12,20.88,25.00,3.28,5.08,4.52,3.67,49
> LoRaWAN package to 0x88 - 75 bytes -> done
  MAIN: Enter Sleep/Wait Mode for 360 sec.
```

Die Logik-Ablauf für die loop() Routine:
1. Get round robin index of new sensor data objekt for (BHDB-Idx)
2. Check: Any Web request reached -> take special action (not supported yet
3. read out time & date from  RTC Module
4. Read new sensor data
	a. HX711: Weigtcell value -> send message if neg. weight jump within 30 min. -> swarm alarm
	b. OW-Bus: all temperature sensors
	c. ADS1115: Power Source values -> ToDo: shutdown if Battery is empty
5. Log Sensor Data to SDCard
6. Log Sensor Data via LoRa WAN / WiFi
7. Update epaper Display by new Sensor + Power data
8. Start DeepSleep mode (or just delay loop with blinking LED for test purpose) for 10Min.
10 back to loop()



### Kalibrierung der Waage
Nach dem Aufbau und erstem Einschalten wird die Waage noch wilde Werte anzeigen, weil der 0 kg Bezug  nicht passend ist.

Im Kapitel: **[AD Wandler HX711](#ad-wandler-hx711)**
haben wir folgende Richtwerte errechnet:
- Der ADC HX711 liefert den Wert: **44**/Gramm.
- Als 1Kg-Scale-Divider sollte man mit dem Wert: **44000/kg** starten.

Nun gilt es noch den scale_Offset Wert zu ermitteln, wie in der header datei HX711Scale.h definiert.

Dazu dient folgende Vorgehensweise:
1. In der Datei: HX711Scale.h den Wert scale_Offset rücksetzen: **#define scale_Offset 0**
2. In Setup() Routine des main.cpp das LogFlag: um LOGHX711 ergänzen, und das Programm neu bauen und starten
3. Ohne jedes Gewicht auf der Waage misst HX711 nun via serial Monitor den adäquaten Wert für das aktuelle Stockwaagen-Deckelgewicht (z.B. 243000 entspricht bei 44000/kg => 5.5kg).
-> HX711: Weight(raw) : 24300 - Weight(unit): 5.523 kg
4. In der Datei: HX711Scale.h den Wert scale_Offset nun mit diesem ersten Raw-Wert setzen.
5. Programm neu bauen und starten
6. Jetzt sollten ca. 0.000kg angezeigt werden und im ser.Monitor der Offsetwert ausgegeben werden
Ideal: HX711: Weight(raw) : 0 - Weight(unit): -0.000 kg
7. Nun ein bekanntes hohes (>50kg) Referenzgewicht auf die Waage stellen.
8. Den HX711-Messwert im ser. Monitor durch das bekannte Gewicht teilen. Das Ergebnis sollte in der Nähe von 44000 liegen, was dem rechnerischen kg Wert entspricht.
9. Diesen "neuen" Wert in HX711Scale als scale_DIVIDER Wert (statt 44000) festlegen
10. In der setup() Routine das LogFlag wieder rücksetzen, Programm neu bauen und starten

Nun sollte ohne jedes Gewicht weiterhin 0,0kg angezeigt werden, und das rerenzgewicht mit dem richtigen kg-Wert.
Die Waage ist nun kalibriert und einsatzbereit

Als Refernzgewicht eignet sich auch ein gut bekanntes eigenes Körpergewicht:
-> Aufsteigen, und Deltawert durch bekanntes Körpergewicht als REFKG angeben.

**Vorteil**: Der Messfehler ist um die Anzahl Kg des Körpergewichts kleiner als nur bei einem kleinen Refernzgewicht.

Da man massgeblich nur an den relativen Messwerten z.B. zur Diagrammdarstellung interessiert ist, ist derartige Kalibrierung nur einmal bei der Erstaufstellung nötig. Danach reicht die relative Aussage als Messkurve.



Soweit der Aufbau der Binenstockwaage. Wenn euch noch weitere Angaben für einen erfolgreichen Aufbau fehlen, lasst es mich wissen. Ansonsten viel Spass dabei...

Hier noch ien paar Addendums:
==============================

### Optional: WebUI Daten Service
Die Daten werden vom Sensorclient zum RPi-Gateway z.B. über LoRaWAN verschlüsselt übermittelt.
Dort werden sie über die Zeit in einer CSV Datei zeilenweise aufgesammelt und es entsteht der Bedarf, diese als Funktions-Diagramme darzustellen.
Will man also diese Messergebnisse als Webpage darstellen (z.B. via Dygraph library) bietet sich als HTTP Service standardmäßig der Apache 2 Webserver an:

Apache 2 Webserver installieren:
1. sudo apt-get update
2. sudo apt-get install apache2
3. Test: http://localhost:80
4. WebHome: /var/www/html 	(Apache 2 default)
5. Server Configuration: /etc/apache2/ports.conf

ToDo: "Hier folgt nun die Beschreibung der Webpage Datei: index.html zur darstellung der Datendiagramme mittels Dygraph-library"

Beipiel:  **http://randolphesser.de/imkerei/index.html**  
=> Menü: Bienenstockwaage 


## WebUI ToDo - Liste
### Ideen-Liste
für eine WebUI Erweiterung (Farbig markierte Vorschläge sind in v1.3 schon umgesetzt):
+ ==Header==
	* Standortangaben zur Beute: -> Headline
		- ==Bild==
		- Standortname Strasse, PLZ, Wegbeschreibung
		- ==Inhaber/Imkerangaben== + Adresse
		- Waagen Unique ID
	* ==Startkonfiguration der Beute zu Messbeginn==
	* Wann letztes Daten-update: Datum & Uhrzeit
	* Besuchszähler: siehe auch [www.andyhoppe.com](http://www.andyhoppe.com/)
+ ==5-Tages Wettervorhersage pro Standort==
	* [**==Holzkirchen Wetterstation==**](http://wetterstationen.meteomedia.de/?station=109680&wahl=vorhersage)
	* DWD joomla plugin -> ftp update
	* ==Varroawetter-Link: Bsp.:==
		- [==**Otterfing**==](http://www.bienenkunde.rlp.de/Internet/global/inetcntr.nsf/dlr_web_full.xsp?src=UK532RPP90&p1=title%3DOtterfing%7E%7Eurl%3D%2FInternet%2FAM%2FNotesBAM.nsf%2FABC_Imker_RLP%2F26E7CFEF10EF5FD5C12574E40023B052%3FOpenDocument&p3=D2KEU5C709&p4=XF10F330RV)
+ ==Tages Min/Max Werte: mit Uhrzeit==
	* ==Gewicht absolut==
	* ==Gewicht Tagesdelta==
	* ==Temp. Min/Max==
+ ==Aktuelle Moment-Werte (aktualisierbar)==
	* ==Aussen-Temperatur==
	* ==Luftfeuchte==
	* ==Beutentemperatur==
	* ==Gesamt-Gewicht==
+ Messwerte Diagramm (auf Lora-Gateway RPi)
	* Mit Tag Nacht Schattierung
	* 2-Tagesfenster:
		- ==Gewicht, Beutentemp. + Aussentemperatur + Luftfeuchte==
	* ==Auswertung mit Monatsfenster (zoombar per button)==
		- ==Temperaturfenster Min/Max pro Tag==
		- ==Gewichtsdelta / Tag==
		- Trendkurve für Gew. + Temp.
		- ==Oder Zeitfenster der Ausgabe einstellbar==
		- ==Aktionsnachricht an Messpunkt einblenden==
	* ==Gewichtsverlauf vertikal (- links <-> rechts +)==
		- ==Horizontal scrollbar an rechter/linker Seite mit Datum & Uhrzeit==
+ Arbeitspage für Völkerdurchsicht
	* Historie der zuletzt durchgeführten Arbeiten -> E-Stockbuch ?
	* Notizenfenster für aktuell geplante Arbeit
		- Bausteine als Listbox
	* Als elektronisches Stockbuch exportierbar/druckbar
	* Jahresstockbuch über alle Völker -> scrollbar
	* Tablet-tauglich
+ ==Konfiguration von Waagen Betriebsparameter -> config.ini File==
	* ==Rekalibrierungsfunktion von Waage und Temp-Sensoren (online ?)==
+ ==Export/Importfunktion==
	* ==aller Mess-/Log Daten==
	* ==Konfigurationseinstellungen==
	* ==Auto mirroring aller Daten auf remote server + backupPfad==
+ Alarm-/Diebstahlfunktion
	* GPS Sensordaten
	* Kombiniert mit Gewichtssprüngen
	* ==Batterie-/PowerSource Alarm==
	* Schwarmalarm ?
	* -> email / SMS ?
+ Thermobild Analyse -> kein Öffnen nötig
	* Best. Volksstärke
	* Sitz
	* Temperaturzustand
+ BeeCounter ?
+ ==7.2Ah Blei Gel Akku (Panasonic)==

## ToDo: Optional HW Module

Wettervorhersage tool: `sudo apt-get install weather-util`
Flughafen München „Franz Josef Strauß“ (IATA-Code: MUC, ICAO-Code: EDDM)
Test with `Weather eddm`

Daten-Transfer und Web Anbindung per GSM Module

Bienen-Stock Aussen-Kamera mit IR: Zur Kontrolle des Volkssitz im Winter


Weitere interessante Messwerte
+ Luftfeuchtigkeit (Grundkonfiguration)
+ ==Außentemperatur (Grundkonfiguration)==
+ ==Temperatur innerhalb der Bienenstöcke==
+ Niederschlagsmenge
+ Luftdruck
+ Windstärke
+ Einbruchalarm
+ ==Spannungsüberwachung der eingebauten 3.7V Batterie
+ ==Spannungsüberwachung der externen 12V Batterie==

Farbig markierte Vorschläge sind in der aktuellen Programmversion v2.0 bereits enthalten.

### Nutzung einer SIM-Karte
Aus: [**Imker-Stockwaage.de**](http://www.imker-stockwaage.de/hardware/simkarte)
Siehe auch unter: *http://www.kasmocom.de*
	und suchen dort unter: [**M2M PrePaid-SIM**](http://www.kasmocom.de/?p=2137)
Ansonsten kann jede Prepaid-SIM verwendet werden.
Es gibt auch kostenlose SIM´s mit kostenlosem Datenvolumen.
Für die BeeIoT Lösung ist eine NB-LTE(IoT)SIM nötig.

## Quellen/Links
### Marktübersicht professioneller Bienenstockwaagen
Professionelle Anbieter digitaler Bienenstockwaagen in der EU:
* BeeWatch (Biene & Natur GmbH)
* BeeWise (aus Frankreich, Hersteller nicht erkenntlich)
* Capaz Bienenwaage (CAPAZ GmbH)
* Funk Bienenstockwaage (WE GRO Engineering)
* GSM-Monitor der Bienenstöcke (Vorlon Technology ltd.)
* Kaptármérleg (auf Ungarisch)
* Livelco (Livelco sp. z o.o.)
* Optilog-b (Borntraeger GmbH)
* Penso Bienenstockwägesystem (Emsystech Engineering)
* SMS Cebelar (aktuell nur auf Slowenisch; verwendet die JShip-Plattform und ist daher nur für den Einsatz in geschützten Umgebungen zu empfehlen)
* Wolf Waagen (Wilhelm Wolf e.K.)
* XLOG bee (Micro El)

Weitere Bienenstockwaage-Projekte im Eigenbau (Hobby- und Open Source-Projekte):
* Beelogger (Arduino)
* Fighting with Technology: Beehive scale build details (auf Englisch; Arduino)
* Hackerbee.com (auf Englisch, work in progress)
* Imker-stockwaage.de (Arduino)
* Mois-Blog Bienenwaage (Arduino)
* OpenBeeLab (auf Französisch)
.
.


Das war es soweit erstmal von meiner Seite.

Viel Spass damit und einen Imkerlichen Gruss
wünscht Euch 

Randolph Esser
(mail(a)RandolphEsser.de)


[**www.RandolphEsser.de**](http://www.RandolphEsser.de/Imkerei)
==> Imkerei
