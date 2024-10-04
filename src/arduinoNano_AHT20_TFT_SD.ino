#include <Adafruit_AHTX0.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>

// Definición de pines para la pantalla TFT y SD
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_BL 6
#define SD_CS 7

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_AHTX0 aht;

const int SCREEN_WIDTH = 128;   // Ancho de la pantalla
const int SCREEN_HEIGHT = 160;  // Alto de la pantalla
const int TEXT_SIZE = 2;        // Tamaño del texto
const int BG_COLOR = ST77XX_BLACK;
const int TEXT_COLOR = ST77XX_ORANGE;
const int RECT_BG_COLOR = ST77XX_BLUE;
const uint16_t GOLD_COLOR = tft.color565(253, 245, 28);  // Color dorado


const long INTERVAL = 300003UL;       // Intervalo de tiempo para actualizar la pantalla (5 minutos)
unsigned long previousMillis = 0;  //  control del tiempo
int data_counter = 0;
float temperature, humidity;

void setup() {
  Serial.begin(9600);
  initDisplay();
  bool sdInitialized = initSDCard();
  bool sensorInitialized = initSensor();

  // Confirmación visual
  tft.fillScreen(BG_COLOR);
  tft.setTextColor(TEXT_COLOR, BG_COLOR);
  tft.setCursor(20, 40);
  tft.print(sdInitialized ? F("SD OK") : F("SD ERROR"));

  tft.setCursor(20, 80);
  tft.print(sensorInitialized ? F("Sensor OK") : F("Sensor ERR"));

  delay(3000);  // Esperar para leer los mensajes
  tft.fillScreen(BG_COLOR);  // Limpiar la pantalla después de la confirmación

  float temperature, humidity;
  readSensorData(temperature, humidity);
  displayData(temperature, humidity, data_counter);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= INTERVAL) {
    previousMillis = currentMillis;
    float temperature, humidity;
    readSensorData(temperature, humidity);
    displayData(temperature, humidity, data_counter);

    // Guardar los datos en la tarjeta SD y medir el tiempo
    //unsigned long startTime = millis();
    saveDataToSD(data_counter, temperature, humidity);
    //unsigned long endTime = millis();
    //Serial.print("Tiempo de registro en SD: ");
    // Serial.print(endTime - startTime);
    //Serial.println(" ms");

    data_counter++;
  }
}

void initDisplay() {
  tft.initR(INITR_GREENTAB);
  tft.setRotation(1);
  tft.fillScreen(BG_COLOR);
  tft.setTextSize(TEXT_SIZE);

  // Configuración del pin de control del brillo
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 128);  // Ajustar el brillo al 50%
}

bool initSDCard() {
  if (!SD.begin(SD_CS)) {
    Serial.println(F("No se pudo inicializar la tarjeta SD."));
    return false;
  }
  Serial.println(F("Tarjeta SD inicializada."));

  if (!SD.exists("datalog.csv")) {
    File dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println(F("Contador,Temperatura(°C),Humedad(%)"));
      dataFile.close();
    } else {
      Serial.println(F("Error al crear datalog.csv"));
      return false;
    }
  }
  return true;
}

bool initSensor() {
  if (!aht.begin()) {
    Serial.println(F("No se pudo encontrar el sensor AHT20."));
    return false;
  }
  Serial.println(F("Sensor AHT20 encontrado."));
  return true;
}

void readSensorData(float &temperature, float &humidity) {
  sensors_event_t humidityEvent, tempEvent;
  aht.getEvent(&humidityEvent, &tempEvent);

  temperature = tempEvent.temperature;
  humidity = humidityEvent.relative_humidity;

  //Serial.print("Temperatura leída: ");
  //Serial.print(temperature);
  //Serial.print(" °C, Humedad: ");
  //Serial.print(humidity);
  //Serial.println(" %");
  Serial.println(F("Datos del sensor actualizados."));
}

void displayData(float temperature, float humidity, uint16_t counter) {
  tft.fillScreen(BG_COLOR);
  displayText(12, 20, F("Temp: "), temperature, true);
  displayText(12, 50, F("Hum: "), humidity, false);
  displayCounter(counter);
  Serial.println(F("Pantalla actualizada con nuevos datos."));
}

void displayText(int16_t x, int16_t y, const __FlashStringHelper *label, float value, bool isTemperature) {
  tft.setTextColor(TEXT_COLOR, BG_COLOR);
  tft.setCursor(x, y);
  tft.print(label);
  tft.print(value, 1);  // Mostrar un decimal

  int16_t cursorX = tft.getCursorX();
  int16_t cursorY = tft.getCursorY();

  if (isTemperature) {
    // Dibujar el símbolo de grado
    tft.fillCircle(cursorX + 2, y + (TEXT_SIZE * 4), 2, ST77XX_BLUE);
    tft.setCursor(cursorX + 8, y);
    tft.print(F("C"));
  } else {
    tft.print(F(" %"));
  }
}

void displayCounter(uint16_t counter) {
  int16_t rectX = 45, rectY = 90, rectW = 70, rectH = 30;
  tft.fillRect(rectX, rectY, rectW, rectH, RECT_BG_COLOR);

  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%d", counter);

  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(TEXT_SIZE);
  tft.getTextBounds(buffer, 0, 0, &x1, &y1, &w, &h);

  int16_t textX = rectX + (rectW - w) / 2;
  int16_t textY = rectY + (rectH + h) / 2 - h;

  tft.setTextColor(GOLD_COLOR, RECT_BG_COLOR);
  tft.setCursor(textX, textY);
  tft.print(buffer);
}

void saveDataToSD(uint16_t counter, float temp, float hum) {
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(counter);
    dataFile.print(',');
    dataFile.print(temp, 1);
    dataFile.print(',');
    dataFile.println(hum, 1);
    dataFile.close();
    Serial.println(F("Datos del sensor guardados en SD."));
  } else {
    Serial.println(F("Error al abrir datalog.csv"));
  }
}
