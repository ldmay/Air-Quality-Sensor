/////////////////////////////////////////////////////////////////////////////
/* 
3D Printer Air Monitor
*/
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/* Config SPS30 interface */
#define SPS30_INTERFACE_UART
//#define SPS30_INTERFACE_I2C
/////////////////////////////////////////////////////////////////////////////

#ifdef SPS30_INTERFACE_I2C
  #include <SensirionI2cSps30.h>

  SensirionI2cSps30 sps;
#elif defined(SPS30_INTERFACE_UART)
  #include <SensirionUartSps30.h>

  #define RX_PIN 5
  #define TX_PIN 18
  #define SENSOR_SERIAL_INTERFACE Serial1

  SensirionUartSps30 sps;
#endif

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "Adafruit_SGP40.h"
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "index_html.h"
#include "Pass.h"

#define SEALEVELPRESSURE_HPA (1013.25) //default pressure at sea level = 1013.25 hPa. Used for BME
#define TIMER_DELAY 1000  //time between sensor measurements in milliseconds

// make sure NO_ERROR has proper definition
#ifdef NO_ERROR  
#undef NO_ERROR
#endif
#define NO_ERROR 0

//sensor object declarations
Adafruit_BME680 bme; 
Adafruit_SGP40 sgp;

//function declarations
void init_wifi();
void init_server();
void init_sgp();
void init_bme();
void init_sps();
void read_sgp();
void read_bme();
void read_sps();
String toString(const String& var);
void sendEvents();

//variable declarations
//Sensor measurements
int32_t vocIndex = 0;
float temperature = 0;
float pressure = 0;
float humidity = 0;
float gasResistance = 0;
float altitude = 0;
struct spsVals{
  uint16_t mc1p0 = 0;
  uint16_t mc2p5 = 0;
  uint16_t mc4p0 = 0;
  uint16_t mc10p0 = 0;
  uint16_t nc0p5 = 0;
  uint16_t nc1p0 = 0;
  uint16_t nc2p5 = 0;
  uint16_t nc4p0 = 0;
  uint16_t nc10p0 = 0;
  uint16_t typicalParticleSize = 0;
} spsVal;

//flags
ulong lastReadTime = 0;
bool bmeInitFail = false;
bool sgpInitFail = false;
bool spsInitFail = false;
bool bmeReadSuccess = false;

//Create web server and set up event source to allow new measurements to be pushed to the server
AsyncWebServer server(80); 
AsyncEventSource events("/events");
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;


/////////////////////////////////////////////////////////////////////////////
/* Setup & Loop */
/////////////////////////////////////////////////////////////////////////////


void setup()
{
  Serial.begin(115200);
  while (!Serial) {delay(10);} //wait for console

  Wire.begin(); //Initialising I2C bus

  init_wifi();
  init_bme();
  init_sgp();
  init_sps();
  init_server();

  lastReadTime = millis();

}

void loop()
{
  //Initiate sensor read loop if time to last read exceeds TIMER_DELAY
  if ((millis() - lastReadTime) > TIMER_DELAY) {
    Serial.println();
    Serial.print("Initiating sensor read. Reading time: ");
    Serial.println(millis());
    Serial.println();

    read_bme();
    read_sgp();
    read_sps();
    lastReadTime = millis();

    sendEvents();
  }
}


/////////////////////////////////////////////////////////////////////////////
/* Initialisation Functions */
/////////////////////////////////////////////////////////////////////////////


void init_wifi() {
  
  //Set the device as a station and soft access point 
  WiFi.mode(WIFI_AP_STA);

  //Set device as a wifi station
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a wifi station ..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());

}


void init_server() {
  //Create web server and configure event source

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html, toString);
  });

  events.onConnect([](AsyncEventSourceClient *client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected. Last message ID received: %u\n",client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();

}


void init_bme() {
  Serial.println("Initialising BME.");

  //Requires BME SDO pin connected to ground, I2C address = 0x76
  if (!bme.begin(0x76)) {
    Serial.println("Could not find BME sensor");
    bmeInitFail = true;
    return;
  }

  //Using the default oversampling parameters
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320,150); 

  bmeInitFail = false; 
}


void init_sgp() {
  Serial.println("Initialising SGP.");

  if (!sgp.begin()){
    Serial.println("Error starting SGP");
    sgpInitFail = true;
    return;
  }

  sgpInitFail = false;
}


void init_sps() {

  Serial.println("Initialising SPS.");

  int8_t serialNumber[32] = {0};
  int8_t productType[8] = {0};
  char spsErrorMessage[64];
  int16_t spsError;

  #ifdef SPS30_INTERFACE_UART
    SENSOR_SERIAL_INTERFACE.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    while (!SENSOR_SERIAL_INTERFACE) {
      delay(100);
    }
    sps.begin(SENSOR_SERIAL_INTERFACE);
  #elif defined(SPS30_INTERFACE_I2C)
    sps.begin(Wire, SPS30_I2C_ADDR_69); 
  #endif

  sps.stopMeasurement();

  spsError = sps.readSerialNumber(serialNumber, 32);
  if (spsError != NO_ERROR) {
    Serial.print("Error trying to execute readSerialNumber(): ");
    errorToString(spsError, spsErrorMessage, sizeof spsErrorMessage);
    Serial.println(spsErrorMessage);
    spsInitFail = true;
    return;
  } 
  Serial.print("serialNumber: ");
  Serial.println((const char*) serialNumber);

  spsError = sps.readProductType(productType, 8); //9 for UART?
  if (spsError != NO_ERROR) {
    Serial.print("Error trying to execute readProductType(): ");
    errorToString(spsError, spsErrorMessage, sizeof spsErrorMessage);
    Serial.println(spsErrorMessage);
    spsInitFail = true;
    return;
  }
  Serial.print("productType: ");
  Serial.println((const char*) productType); //convert productType ASCII to string


  #ifdef SPS30_INTERFACE_UART
    spsError = sps.startMeasurement((SPS30OutputFormat)(261));
  #elif defined(SPS30_INTERFACE_I2C)
    spsError = sps.startMeasurement((SPS30OutputFormat)(1280)); 
  #endif

  if (spsError != NO_ERROR) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(spsError,spsErrorMessage, sizeof spsErrorMessage);
    Serial.println(spsErrorMessage);
    spsInitFail = true;
    return;
  }

  delay(100);
  Serial.println("Successfully started SPS30 measurements.");
  spsInitFail = false;

  spsError = sps.startFanCleaning();

  if (spsError != NO_ERROR) {
    Serial.print("Error trying to execute startFanCleaning(): ");
    errorToString(spsError,spsErrorMessage, sizeof spsErrorMessage);
    Serial.println(spsErrorMessage);
    spsInitFail = true;
    return;
  }
  Serial.println("Fan cleaning done.");

}


/////////////////////////////////////////////////////////////////////////////
/* Read Functions */
/////////////////////////////////////////////////////////////////////////////


void read_bme() {

  if (bmeInitFail) {
    Serial.println("BME init fail flag on. Sensor read skipped.");
    return;
  }

  Serial.println("Reading BME");

  ulong time = bme.beginReading(); 
  bmeReadSuccess = false;

  if (time == 0) {
    Serial.println("BME sensor failed to begin reading.");
    return;
  }

  if (!bme.endReading()) {
    Serial.println("BME680 sensor failed to complete reading.");
    return;
  } else { bmeReadSuccess = true; }

  temperature = bme.temperature;
  pressure = bme.pressure / 100.0;
  humidity = bme.humidity;
  gasResistance = bme.gas_resistance / 1000.0;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.println("C");
  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println("hPa");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  Serial.print("Gas Resistance: ");
  Serial.print(gasResistance);
  Serial.println(" kOhms");
  Serial.print("Approx Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");
  Serial.println();

}


void read_sgp() {

  if (sgpInitFail) {
    Serial.println("SGP init fail flag on. Sensor read skipped.");
    return;
  }

  Serial.println("Reading SGP");

  //test SGP sensor
  bool test;
  test = sgp.selfTest();
  Serial.printf("SGP self-test result: %d", test);
  Serial.println();

  //SGP sensor requires temp and humidity data. Only read when this is available
  if (bmeReadSuccess) {
    //Parameters: Temperature in C, Relative humidity in % rH
    //Returns: VOC Index
    vocIndex = sgp.measureVocIndex(bme.temperature, bme.humidity);
  } else {
    Serial.println("SGP sensor read failed. No BME data available.");
    return;
  }

  Serial.print("VOC Index: ");
  Serial.println(vocIndex);
  Serial.println();
}


void read_sps(){
  
  if (spsInitFail) {
    Serial.println("SPS init fail flag on. Sensor read skipped.");
    return;
  }

  Serial.println("Reading SPS");

  char spsErrorMessage[64];
  int16_t spsError;
  uint32_t deviceStatus;

  #ifdef SPS30_INTERFACE_I2C
    sps.readDeviceStatusRegister(deviceStatus);
    Serial.print("Device Status: ");
    Serial.println(deviceStatus);

      uint16_t dataReadyFlag = 0;
    //data ready flag indicates new measurements are available
    spsError = sps.readDataReadyFlag(dataReadyFlag);
    if (spsError != NO_ERROR) {
      Serial.print("Error trying to execute readDataReadyFlag(): ");
      errorToString(spsError, spsErrorMessage, sizeof spsErrorMessage);
      Serial.println(spsErrorMessage);
      return;
    }
    Serial.print("dataReadyFlag: ");
    Serial.println(dataReadyFlag);

    //take new measurements when new readings are available
    //if (dataReadyFlag) {
  #elif defined(SPS30_INTERFACE_UART)  
    uint8_t reserved1;
    sps.readDeviceStatusRegister(false, deviceStatus, reserved1);
    Serial.print("Device Status: ");
    Serial.println(deviceStatus);

    if (1) {
      spsError = sps.readMeasurementValuesUint16(spsVal.mc1p0, spsVal.mc2p5, spsVal.mc4p0, spsVal.mc10p0, 
                                                spsVal.nc0p5, spsVal.nc1p0, spsVal.nc2p5, spsVal.nc4p0,
                                                spsVal.nc10p0, spsVal.typicalParticleSize);
      if (spsError != NO_ERROR) {
        Serial.print("Error trying to execute readMeasurementValuesUint16(): ");
        errorToString(spsError, spsErrorMessage, sizeof spsErrorMessage);
        Serial.println(spsErrorMessage);
        return;
      }
    }
    uint32_t deviceStatusRegister;
    uint8_t reserved2;

    spsError = sps.readDeviceStatusRegister(false, deviceStatusRegister, reserved2);
    if(spsError != NO_ERROR) {
      Serial.print("Error trying to execute readDeviceStatusRegister(): ");
      errorToString(spsError, spsErrorMessage, sizeof spsErrorMessage);
      Serial.println(spsErrorMessage);
      return;
    }
    Serial.printf("Device Status Register: %d",deviceStatusRegister);
    Serial.println();
  #endif
  
  Serial.print("Mass Concentration in ug/m^3              ");
  Serial.print("PM  1.0: ");
  Serial.print(spsVal.mc1p0);
  Serial.print("  PM  2.5: ");
  Serial.print(spsVal.mc2p5);
  Serial.print("  PM  4.0: ");
  Serial.print(spsVal.mc4p0);
  Serial.print("  PM 10.0: ");
  Serial.println(spsVal.mc10p0);
  Serial.print("Number Concentration in #/cm^3");
  Serial.print("NC  0.5: ");
  Serial.print(spsVal.nc0p5);
  Serial.print("  NC  1.0: ");
  Serial.print(spsVal.nc1p0);
  Serial.print("  NC  2.5: ");
  Serial.print(spsVal.nc2p5);
  Serial.print("  NC  4.0: ");
  Serial.print(spsVal.nc4p0);
  Serial.print("  NC 10.0: ");
  Serial.println(spsVal.nc10p0);
  Serial.print("Typical partical size: ");
  Serial.println(spsVal.typicalParticleSize);

}


/////////////////////////////////////////////////////////////////////////////
/* Helper Functions */
/////////////////////////////////////////////////////////////////////////////


void sendEvents() {
  //Send sensor readings to the web server
  events.send("ping",NULL,millis());
  events.send(String(temperature).c_str(), "temperature", millis());
  events.send(String(humidity).c_str(),"humidity",millis());
  events.send(String(pressure).c_str(), "pressure", millis());
  events.send(String(gasResistance).c_str(),"gas",millis());
  events.send(String(vocIndex).c_str(),"voc",millis());
  events.send(String(spsVal.typicalParticleSize).c_str(),"particle",millis());
  events.send(String(spsVal.mc1p0).c_str(), "mc1p0", millis());
  events.send(String(spsVal.mc2p5).c_str(), "mc2p5", millis());
  events.send(String(spsVal.mc4p0).c_str(), "mc4p0", millis());
  events.send(String(spsVal.mc10p0).c_str(), "mc10p0", millis());
  events.send(String(spsVal.nc0p5).c_str(), "nc0p5", millis());
  events.send(String(spsVal.nc1p0).c_str(), "nc1p0", millis());
  events.send(String(spsVal.nc2p5).c_str(), "nc2p5", millis());
  events.send(String(spsVal.nc4p0).c_str(), "nc4p0", millis());
  events.send(String(spsVal.nc10p0).c_str(), "nc10p0", millis());
}


String toString(const String& var) {
  //replaces placeholders in HTML code with real values before sending it to server
  
  if (var == "TEMPERATURE") {
    return String(temperature);
  }
  else if (var == "HUMIDITY") {
    return String(humidity);
  }
  else if (var == "PRESSURE") {
    return String(pressure);
  }
  else if (var == "GAS") {
    return String(gasResistance);
  }
  else if (var == "VOC") {
    return String(vocIndex);
  }
  else if (var == "PARTICLE") {
    return String(spsVal.typicalParticleSize);
  }
  return "-";

}





