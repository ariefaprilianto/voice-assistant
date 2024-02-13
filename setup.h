#include "global.h"
#include "config.h"

#include "recording.h"
#include "processing.h"
#include "playing.h"
#include "base64_wrapper.h"
#include "other.h"
#include "Google_Wrapper.h"

WiFiClientSecure *googleWifiClient = nullptr;
WiFiClientSecure *openAIWifiClient = nullptr;

SPIClass *hspi = NULL;
bool initSDCard() {
  Serial.print("Initializing SD card.......... ");
  hspi = new SPIClass(HSPI);
  hspi->begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, *hspi)) {
     Serial.print("Failed to init SD card.......... ");
    return false;
  }

  if (SD.exists(samplesRecordedFilePath)) {SD.remove(samplesRecordedFilePath);}
  if (SD.exists(samplesRecordedFilePath_base64)) {SD.remove(samplesRecordedFilePath_base64);}
  if (SD.exists(samplesGoogleFilePath)) {SD.remove(samplesGoogleFilePath);}
  if (SD.exists(samplesGoogleFilePath_base64)) {SD.remove(samplesGoogleFilePath_base64);}
  if (SD.exists(jsonCallFilePath)) {SD.remove(jsonCallFilePath);}
  if (SD.exists(jsonResponseFilePath)) {SD.remove(jsonResponseFilePath);}
  return true;
}

bool connectToWiFi() {
  Serial.print("Connecting to Wi-Fi........... ");

  WiFi.begin(SSID, PASSWORD);
  WiFi.setAutoReconnect(true);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 20000) { // Timeout after 20 seconds
      return false;
    }
    delay(1000);
  }
  return true;
}

bool i2s_mic_install() {
  Serial.print("Installing I2S (Mic) ......... ");
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE * 8, //after averaging and considering processor timing, 64200 I2S sample rate is aprx 8000hz
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 128,
    .dma_buf_len = I2S_READ_LEN,
    .use_apll = false
  };
  esp_err_t i2s_install_status = i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  esp_err_t i2s_pin_status = i2s_set_pin(I2S_NUM_1, &pin_config);
  
  if (i2s_install_status == ESP_OK && i2s_pin_status == ESP_OK){
    Serial.println("i2s installation succeeded!");
    return true;
  }
}

bool i2s_speaker_install() {
  Serial.print("Installing I2S (Speaker) ..... ");

  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = I2S_SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S_MSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 1024,
      .use_apll = false,
      .tx_desc_auto_clear = true,
  };
  esp_err_t i2s_install_status = i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_config = {
      .bck_io_num = STD_BCLK,
      .ws_io_num = STD_WS,
      .data_out_num = STD_DOUT,
      .data_in_num = I2S_PIN_NO_CHANGE};

  esp_err_t i2s_pin_status = i2s_set_pin(I2S_NUM_1, &pin_config);
  i2s_set_sample_rates(I2S_NUM_1, I2S_SAMPLE_RATE);

  if (i2s_install_status == ESP_OK && i2s_pin_status == ESP_OK){
    Serial.println("Success!");
    return true;
  }
}

void initWifiClient() {
  // Initialize WiFi client Google
  googleWifiClient = new WiFiClientSecure;
  if (googleWifiClient) {
      googleWifiClient->setCACert(GOOG_ROOT_CA);
  }

  // Initialize WiFi client OpenAI
  openAIWifiClient = new WiFiClientSecure;
  if (openAIWifiClient) {
      openAIWifiClient->setInsecure();
  }
}

// -------------------------------------------------------

bool start(){
  if (!initSDCard()) {
    Serial.println("Failed. Exiting...");
    return false;
  }
  Serial.println("SD card initialized!");

  if (!connectToWiFi()) {
    Serial.println("Failed. Exiting...");
    return false;
  }
  initWifiClient();
  Serial.println("WiFi connected!");  
  return true;
}

