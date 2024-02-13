#include "playing.h"
#include "global.h"
#include "config.h"

struct WavHeader {
  uint32_t sample_rate;
  uint16_t bits_per_sample;
  uint16_t num_channels;
};

bool parseWavHeader(File &file, WavHeader &header) {
  if (file.read() != 'R' || file.read() != 'I' || file.read() != 'F' || file.read() != 'F') {
    Serial.println("Invalid WAV file");
    return false;
  }

  file.seek(22);
  header.num_channels = file.read() | (file.read() << 8);
  header.sample_rate = file.read() | (file.read() << 8) | (file.read() << 16) | (file.read() << 24);
  file.seek(34);
  header.bits_per_sample = file.read() | (file.read() << 8);
  file.seek(44);  // Skip to the audio data
  return true;
}

void playAudioSamples(std::string filename) {
  Serial.println("Started playing audio...");
  File file = SD.open(filename.c_str());
  if (!file) {
    Serial.println("Failed to open file for reading");
    while (1) delay(1);
  }

  WavHeader header;
  if (!parseWavHeader(file, header)) {
    Serial.println("Invalid WAV file");
    return;
  }

  i2s_set_sample_rates(I2S_NUM_1, header.sample_rate);

  const size_t buffer_size = 512;
  uint8_t buffer[buffer_size];
  size_t bytes_read;
  size_t bytes_written;
  while (file.available() && (bytes_read = file.read(buffer, buffer_size)) > 0) {
    i2s_write(I2S_NUM_1, buffer, bytes_read, &bytes_written, portMAX_DELAY);
    if (bytes_read == 0) break;
  }

  file.close();
}
