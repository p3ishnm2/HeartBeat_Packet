#include <M5StickCPlus.h>
#include <BLEDevice.h> // Bluetooth Low Energy
#include <BLEServer.h> // Bluetooth Low Energy
#include <BLEUtils.h>  // Bluetooth Low Energy
#include <esp_sleep.h>

/*  Getting_BPM_to_Monitor prints the BPM to the Serial Monitor, using the least lines of code and PulseSensor Library.
 *  Tutorial Webpage: https://pulsesensor.com/pages/getting-advanced
 *
--------Use This Sketch To------------------------------------------
1) Displays user's live and changing BPM, Beats Per Minute, in Arduino's native Serial Monitor.
2) Print: "♥  A HeartBeat Happened !" when a beat is detected, live.
2) Learn about using a PulseSensor Library "Object".
4) Blinks LED on PIN 13 with user's Heartbeat.
--------------------------------------------------------------------*/

#define T_PERIOD 4                // アドバタイジングパケットを送る秒数
#define S_PERIOD 1                // Deep Sleepする秒数 T+S秒ごとに送ることになる
RTC_DATA_ATTR static uint8_t seq; // 送信SEQ

#define USE_ARDUINO_INTERRUPTS true // Set-up low-level interrupts for most acurate BPM math.
#include <PulseSensorPlayground.h>  // Includes the PulseSensorPlayground Library.
#include "xbm.h"                    //my bitmap

//  Variables
const int PulseWire = 32; // PulseSensor PURPLE WIRE connected to ANALOG PIN 0
// const int LED13 = 13;     // The on-board Arduino LED, close to PIN 13.
int Threshold = 550; // Determine which Signal to "count as a beat" and which to ignore.
                     // Use the "Gettting Started Project" to fine-tune Threshold Value beyond default setting.
                     // Otherwise leave the default "550" value.
uint16_t myBPM;
uint16_t vbat; // 電圧

PulseSensorPlayground pulseSensor; // Creates an instance of the PulseSensorPlayground object called "pulseSensor"

void setAdvData(BLEAdvertising *pAdvertising)
{ // アドバタイジングパケットを整形する
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

  oAdvertisementData.setFlags(0x06); // BR_EDR_NOT_SUPPORTED | General Discoverable Mode
  // oAdvertisementData.setFlags(0x05); // BR_EDR_NOT_SUPPORTED | Limited Discoverable Mode

  std::string strServiceData = "";
  strServiceData += (char)0x08;                  // 長さ（12Byte → 7Byte 0x07の方がいいかも）: 0
  strServiceData += (char)0xff;                  // AD Type 0xFF: Manufacturer specific data : 1
  strServiceData += (char)0xff;                  // Test manufacture ID low byte : 2
  strServiceData += (char)0xff;                  // Test manufacture ID high byte : 3
  strServiceData += (char)seq;                   // シーケンス番号 : 4
  strServiceData += (char)(myBPM & 0xff);        // 心拍数の下位バイト（下位を前にして保存するリトルエンディアン形式） : 5
  strServiceData += (char)((myBPM >> 8) & 0xff); // 心拍数の上位バイト : 6
  // strServiceData += (char)(humid & 0xff);        // 湿度の下位バイト
  // strServiceData += (char)((humid >> 8) & 0xff); // 湿度の上位バイト
  // strServiceData += (char)(press & 0xff);        // 気圧の下位バイト
  // strServiceData += (char)((press >> 8) & 0xff); // 気圧の上位バイト
  strServiceData += (char)(vbat & 0xff);        // 電池電圧の下位バイト
  strServiceData += (char)((vbat >> 8) & 0xff); // 電池電圧の上位バイト

  oAdvertisementData.addData(strServiceData);
  pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup()
{
  M5.begin(); //(LCD有効, POWER有効, Serial有効)
  // M5.Axp.begin(false, false, false, false, true); // DCDC3(5V)をOFFにしたい 今のバージョンではできない？
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setRotation(2);

  Serial.begin(115200); // For Serial Monitor
  // アナログ入力設定
  pinMode(PulseWire, ANALOG); //アナログ入力

  // G36とG25は同時使用不可。使っていない方は以下のようにフローティング入力にする
  gpio_pulldown_dis(GPIO_NUM_25);
  gpio_pullup_dis(GPIO_NUM_25);
  // Configure the PulseSensor object, by assigning our variables to it.
  pulseSensor.analogInput(PulseWire); // デフォはG0(A0)
  // pulseSensor.blinkOnPulse(26);       // auto-magically blink Arduino's LED with heartbeat.
  pulseSensor.setThreshold(Threshold);
  // Double-check the "pulseSensor" object was created and "began" seeing a signal.
  if (pulseSensor.begin())
  {
    Serial.println("We created a pulseSensor Object !"); // This prints one time at Arduino power-up,  or on Arduino reset.
  }
  // ここからBLE通信
  BLEDevice::init("blepub-01");                   // デバイスを初期化
  BLEServer *pServer = BLEDevice::createServer(); // サーバーを生成

  BLEAdvertising *pAdvertising = pServer->getAdvertising(); // アドバタイズオブジェクトを取得
  // setAdvData(pAdvertising);                                 // アドバタイジングデーターをセット

  // pAdvertising->start();  // アドバタイズ起動
  // delay(T_PERIOD * 1000); // T_PERIOD秒アドバタイズする
  // // delay(20); // considered best practice in a simple sketch.
  // pAdvertising->stop(); // アドバタイズ停止

  // seq++; // シーケンス番号を更新
  // delay(10);
  // esp_deep_sleep(1000000LL * S_PERIOD); // S_PERIOD秒Deep Sleepする
}

void loop()
{
  myBPM = pulseSensor.getBeatsPerMinute(); // Calls function on our pulseSensor object that returns BPM as an "int".
                                           // "myBPM" hold this BPM value now.

  // if (pulseSensor.sawStartOfBeat())
  // { // Constantly test to see if "a beat happened".
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawXBitmap(0, 20, hb2_bmp, 64, 32, TFT_RED);
  M5.Lcd.setCursor(0, 65);
  Serial.println("♥  A HeartBeat Happened ! "); // If test is "true", print a message "a heartbeat happened".
  Serial.print("BPM:");                         // Print phrase "BPM: "
  Serial.println(myBPM);                        // Print the value inside of myBPM.
  // M5.Lcd.printf("♥  A HeartBeat Happened ! \n");
  M5.Lcd.printf("BPM:%d\n", myBPM);

  if (myBPM > 80)
  {
    int loss = (myBPM - 80) * 4;
    if (loss > 70)
    {
      loss = 70;
    }
    M5.Lcd.setCursor(0, 100);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("LOSS:%d%%\n", loss);
    M5.Lcd.setTextSize(3);
  }

  vbat = (uint16_t)(M5.Axp.GetVbatData() * 1.1 / 1000 * 100); // バッテリーの電圧（100倍して小数点以下を整数部へ）

  BLEServer *pServer = BLEDevice::createServer();           // サーバーを生成
  BLEAdvertising *pAdvertising = pServer->getAdvertising(); // アドバタイズオブジェクトを取得
  setAdvData(pAdvertising);                                 // アドバタイジングデーターをセット

  pAdvertising->start();  // アドバタイズ起動
  delay(T_PERIOD * 1000); // T_PERIOD秒アドバタイズする
  // delay(20); // considered best practice in a simple sketch.
  pAdvertising->stop(); // アドバタイズ停止

  seq++; // シーケンス番号を更新
  // delay(10);
  delay(S_PERIOD * 1000); // considered best practice in a simple sketch.

  // reset Btn
  if (M5.BtnB.wasPressed())
  {
    esp_restart();
  }
  M5.update();
}
