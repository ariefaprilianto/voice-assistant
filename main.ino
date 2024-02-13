#include "setup.h"

enum SystemState {
  IDLE,
  LISTENING,
  PROCESSING,
  RESPONDING
};

SystemState currentState = IDLE;
bool i2sTaskCreated = false;
String transcription;
String answer;

unsigned long previousMillis = 0;
const long interval = 500;
bool ledState = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("---------- Starting ------------ ");
  if (!start()) {
    Serial.println("Initialization failed. Halting.");
    while (true) {}; // Halt the program if initialization fails
  }
  i2s_mic_install(); //Install I2S microphone driver

  pinMode(START_BTN_IO, INPUT);
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
}

void loop() {
  switch (currentState) {
    case IDLE:
      displayStandBy();
      // Check if the start button is pressed
      if (digitalRead(START_BTN_IO) == HIGH && !i2sTaskCreated) {
        currentState = LISTENING;
      }
      i2sTaskCreated = false; // Reset the flag when returning to IDLE
      break;

    case LISTENING:
      displayListening();
      if (!i2sTaskCreated) {
        i2sTaskCreated = true;
        Serial.println("Entering listening mode!");
        i2s_adc();
      }
      currentState = PROCESSING;
      break;

    case PROCESSING:
      displayProcessing();
      // Process the recorded voice
      if (uploadFileToGCS(samplesRecordedFilePath)) {
        Serial.println("Uploaded into GCS successfully.");

        transcription = speechToText(); // Transcribe audio to text
        Serial.print("transcription: ");
        Serial.println(transcription);
      }
      if (transcription.length() > 0 && transcription != "null") {
        answer = getAnswer(transcription);
        Serial.print("answer: ");
        Serial.println(answer);
        currentState = RESPONDING;
      } else {
        currentState = IDLE; // Go back to IDLE if transcription fails
      }
      break;

    case RESPONDING:
      if(textToSpeech(answer)) {
        displayReporting();
        // Respond with voice output
        i2s_driver_uninstall(I2S_NUM_1); // Switch from microphone to speaker
        i2s_speaker_install();
        playAudioSamples(samplesGoogleFilePath);
        i2s_driver_uninstall(I2S_NUM_1); // Switch back to microphone
        i2s_mic_install();
      }
      currentState = IDLE; // Go back to IDLE after responding
      break;
  }

  delay(100); // Short delay to prevent overwhelming the processor
}


void displayStandBy() {
  setColor(0, 0, 255); // Blue color, blinking every 500ms
}

void displayListening() {
  blinkColor(255, 165, 0); // Orange color
}

void displayProcessing() {
  blinkColor(128, 0, 128); // Purple color, blinking every 500ms
}

void displayReporting() {
  blinkColor(0, 255, 0); // Green color, blinking every 500ms
}

void blinkColor(int red, int green, int blue) {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off, turn it on and vice-versa:
    if (ledState) {
      setColor(0, 0, 0); // Turn off the LED
    } else {
      setColor(red, green, blue); // Set the LED color
    }

    // reverse the state of the LED
    ledState = !ledState;
  }
}

void setColor(int red, int green, int blue) {
  analogWrite(RGB_RED_PIN, red);
  analogWrite(RGB_GREEN_PIN, green);
  analogWrite(RGB_BLUE_PIN, blue);
}
