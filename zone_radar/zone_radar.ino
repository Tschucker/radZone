#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include "Button2.h"
#include "esp_adc_cal.h"
#include <HardwareSerial.h>
#include "ld303-protocol.h"

static LD303Protocol protocol;
static bool debug = false;
HardwareSerial SerialPort(1);  //if using UART1

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34

#define BUTTON_1            35
#define BUTTON_2            0

int vref = 1100;
int btnCick = false;

TFT_eSPI tft = TFT_eSPI();
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void showVoltage()
{
    static uint64_t timeStamp = 0;
    if (millis() - timeStamp > 1000) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        String voltage = String(battery_voltage) + "V";
        //Serial.println(voltage);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(voltage,  0, 0, 2);
    }
}

void button_init()
{
    btn1.setPressedHandler([](Button2 & b) {
        Serial.println("btn 1");
        btnCick = true;
    });

    btn2.setPressedHandler([](Button2 & b) {
        Serial.println("btn 2");
        btnCick = true;
    });
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}

void setup() {
  // put your setup code here, to run once:
  //Serial Output
  Serial.begin(115200);
  Serial.println("Start");

  //Serial input from Radar
  SerialPort.begin(115200, SERIAL_8N1, 15, 13);

  /*
  ADC_EN is the ADC detection enable port
  If the USB port is used for power supply, it is turned on by default.
  If it is powered by battery, it needs to be set to high level
  */
  pinMode(ADC_EN, OUTPUT);
  digitalWrite(ADC_EN, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Zone Radar", tft.width() / 2, tft.height() / 2 , 4);
  tft.drawString("by Tea & Tech Time", tft.width() / 2, tft.height() / 2 +32);
  espDelay(1000);
  tft.fillScreen(TFT_BLACK);

  button_init();

  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);    //Check type of calibration value used to characterize ADC
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
  } else {
    Serial.println("Default Vref: 1100mV");
  }


}

void loop() {
  // put your main code here, to run repeatedly:
  if (btnCick) {
    //do something
    Serial.println("Button pressed");
    btnCick = false;
  }

  showVoltage();

  uint8_t buf[256];
  // process incoming data from radar
  while (SerialPort.available()) {
    uint8_t c = SerialPort.read();

    // run receive state machine
    bool done = protocol.process_rx(c);
    if (done) {
      int len = protocol.get_data(buf);
      //printhex("<", buf, len);
      uint8_t cmd = buf[0];
      switch (cmd) {
      case 0xD3:
        int dist = (buf[1] << 8) + buf[2];
        int pres = buf[4];
        int k = (buf[5] << 8) + buf[6];
        int micro = buf[7];
        int off = buf[8];
        printf("distance=%3d,present=%d,strength=%5d,micro=%d,off=%d\n", dist, pres, k, micro, off);
        tft.setTextDatum(MC_DATUM);
        //tft.fillScreen(TFT_BLACK);
        tft.fillRoundRect(tft.width() / 4, tft.height() / 4, tft.width() / 2, tft.height() / 2, 5, TFT_OLIVE);
        tft.setTextColor(TFT_WHITE, TFT_OLIVE);
        tft.drawString(String(dist), tft.width() / 2, tft.height() / 2 , 7);
        break;
      }
    }
  }

  button_loop();  
}
