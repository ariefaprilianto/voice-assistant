#include "global.h"
#include "config.h"

const int headerSize = 44;

// functions to write int, 4 bytes to file
void writeInt(File &file, uint32_t value) {
  file.write((value >> 0) & 0xFF);
  file.write((value >> 8) & 0xFF);
  file.write((value >> 16) & 0xFF);
  file.write((value >> 24) & 0xFF);
}
//functions to write short, 2 bytes to file
void writeShort(File &file, uint16_t value) {
  file.write((value >> 0) & 0xFF);
  file.write((value >> 8) & 0xFF);
}

//write WAV header to .wav file in sd card
void writeWavHeader(File &file, int sampleRate) {
  const int bitsPerSample = I2S_SAMPLE_BITS;
  const int channels = I2S_CHANNEL_NUM;
  uint32_t totalAudioLen = file.size() - 44;  //wav headers are 44 length
  uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;

  file.seek(0);
  // RIFF chunk descriptor
  file.write((uint8_t *)"RIFF", 4);
  writeInt(file, 36 + totalAudioLen);  //chunk size (36 + SubChunk2Size)
  file.write((uint8_t *)"WAVE", 4);

  // fmt sub-chunk (formatting chunk)
  file.write((uint8_t *)"fmt ", 4);
  writeInt(file, 16);                              // Subchunk1Size (16 for PCM)
  writeShort(file, 1);                             // AudioFormat (1 for PCM)
  writeShort(file, channels);                      // NumChannels
  writeInt(file, sampleRate);                      // SampleRate
  writeInt(file, byteRate);                        // ByteRate
  writeShort(file, channels * bitsPerSample / 8);  // BlockAlign
  writeShort(file, bitsPerSample);                 // BitsPerSample

  // data sub-chunk
  file.write((uint8_t *)"data", 4);
  writeInt(file, totalAudioLen);  // Subchunk2Size
}

void example_disp_buf(uint8_t *buf, int length) {
  printf("======\n");
  for (int i = 0; i < length; i++) {
    printf("%02x ", buf[i]);
    if ((i + 1) % 8 == 0) {
      printf("\n");
    }
  }
  printf("======\n");
}

String audioTranscription() {
  
}

void i2s_adc() {
  File samplesFile = SD.open(samplesRecordedFilePath, FILE_WRITE);

  if (!samplesFile) {
    Serial.println("Failed to open file for writing");
    return;
  }

  int i2s_read_len = I2S_READ_LEN;
  int counter = I2S_SAMPLE_RATE * (MAX_DURATION_MS / 1000);
  int16_t sBuffer[i2s_read_len];

  Serial.println("Recording started!");
  float startTime = millis();
  while (counter > 0) {
    // in buffer from I2S audio data
    size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_NUM_1, &sBuffer, i2s_read_len, &bytesIn, portMAX_DELAY);

    if (result == ESP_OK) {
      // Read I2S data buffer
      int16_t samples_read = bytesIn / 8;
      if (samples_read > 0) {
        int16_t mean = 0;
        for (int16_t i = 0; i < samples_read; ++i) {
          mean += (sBuffer[i] * 15);
        }
        // Average the data reading (average of 2 samples)
        mean /= samples_read;
        samplesFile.write((uint8_t *)&mean, sizeof(mean));  //write into .wav sd card file
      }
    }
    counter = counter - 1;
  }
  float time = millis() - startTime;
  int realSampleRate = ((I2S_SAMPLE_RATE * (MAX_DURATION_MS / 1000)) / time) * 1000;
  Serial.println(realSampleRate);
  writeWavHeader(samplesFile, realSampleRate);
  samplesFile.close();
  Serial.println("Recording is done!");
}