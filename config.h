#ifndef CONFIG_H
#define CONFIG_H

#define I2S_SAMPLE_RATE 16000     // Sample rate in Hz
#define I2S_SAMPLE_BITS 16        // Bit depth per sample
#define I2S_READ_LEN 16 // Buffer length for reading data
#define MAX_DURATION_MS 5000     // Maximum duration in milliseconds
#define I2S_CHANNEL_NUM 1       // Number of audio channels
#define I2S_SD 1
#define I2S_WS 2
#define I2S_SCK 3
#define STD_WS 5 // I2S word select io number   I2S_LRC
#define STD_BCLK 4 // I2S bit clock io number   I2S_BCLK
#define STD_DOUT 6 // I2S data out io number    I2S_DOUT
#define START_BTN_IO 16
#define SD_CS_PIN 17
#define SD_CLK_PIN  13
#define SD_MOSI_PIN 11
#define SD_MISO_PIN 12
#define RGB_RED_PIN 47
#define RGB_GREEN_PIN 21
#define RGB_BLUE_PIN 20
#define samplesRecordedFilePath "/samples_recorded.wav"
#define samplesRecordedFilePath_base64 "/samples_recorded_base64.txt"
#define samplesGoogleFilePath "/audio.wav"
#define samplesGoogleFilePath_base64 "/samples_Google_base64.txt"
#define jsonCallFilePath "/json_call.txt"
#define jsonResponseFilePath "/json_response.txt"
#define SSID "Ariefap"
#define PASSWORD "bismillah070514"
#define APIKEYGOOGLE "AIzaSyCQzSts39rDMKNEbbLN7Ga0A9UwIUchuNs"
#define APIKEYOPENAI "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"
#define REFRESHTOKENGOOGLE "1//04Dz3E1pUZ6pPCgYIARAAGAQSNwF-L9IrfovW33Jk-SNPbvSuFfcH6PRQW81GLwte7J5024PY8D4hRpZ6Wa1-QWuPW6oWzeHn8aY"
#define CLIENTSECRETGOOGLE "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"
#define CLIENTIDGOOGLE "377046442100-b388tedrkv9fjpdn0mbghr2s70kokeuf.apps.googleusercontent.com"
#define PROJECT_ID "savvy-container-411411"                                           // Taken from "project_id" key in JSON file.
#define CLIENT_EMAIL "esp32-dev-kit1@savvy-container-411411.iam.gserviceaccount.com"  // Taken from "client_email" key in JSON file.
#define GOOGLE_API_HOST "www.googleapis.com"
inline const char* GCS_BUCKET = "audio-rec-1307";
inline const char* GOOG_ROOT_CA= \
     "-----BEGIN CERTIFICATE-----\n" \
"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\n" \
"bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n" \
"-----END CERTIFICATE-----\n";

#endif // CONFIG_H
