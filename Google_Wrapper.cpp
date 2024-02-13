#include <Arduino.h>
#include "base64.hpp"
#include "Google_Wrapper.h"
#include "SpiRamAllocator.h"
#include "global.h"
#include "config.h"

bool uploadFileToGCS(const char* fileName) {
  const char* headerKeysToCollect[] = { "Location" };
  const size_t headerKeysToCollectCount = sizeof(headerKeysToCollect) / sizeof(headerKeysToCollect[0]);

  String accessToken = getOAuthToken();

  HTTPClient https;
  if (!https.begin(*googleWifiClient, "https://" + String(GOOGLE_API_HOST) + "/upload/storage/v1/b/" + String(GCS_BUCKET) + "/o?uploadType=resumable&name=" + String(fileName).substring(1))) {
    Serial.println("Unable to connect");
    return false;
  }
  https.setTimeout(10000);  // Set timeout to 10000 milliseconds (10 seconds)
  https.collectHeaders(headerKeysToCollect, headerKeysToCollectCount);
  https.addHeader("Authorization", "Bearer " + accessToken);
  https.addHeader("Content-Type", "application/json");
  https.addHeader("X-Upload-Content-Type", "audio/wav");
  https.addHeader("Content-Length", "0");

  int httpCode = https.POST("");
  Serial.println("Status Code: " + String(httpCode));

  String location;
  if (httpCode > 0) {
    // Print all headers for debugging
    Serial.println("Response Headers:");
    for (size_t i = 0; i < https.headers(); i++) {
      Serial.println(https.headerName(i) + ": " + https.header(i));
    }

    // Get Location header
    location = https.header("Location");
    Serial.println("Location Header: " + location);
  } else {
    Serial.println("Error in initial POST: " + https.errorToString(httpCode));
    return false;
  }

  https.end();

  const size_t chunkSize = 262144;  // Set chunk size to 256 KiB
  uint8_t* buffer = (uint8_t*)ps_malloc(chunkSize);
  if (!buffer) {
    Serial.println("Failed to allocate memory for buffer");
    return false;
  }

  File file = SD.open(fileName);
  if (!file) {
    Serial.println("Failed to open file for reading");
    if (buffer) {
      free(buffer);
    }
    return false;
  }

  size_t fileSize = file.size();
  size_t readBytes;
  size_t offset = 0;
  bool success = true;

  while ((readBytes = file.read(buffer, chunkSize)) > 0 && success) {
    const char* headerKeysToCollectChunk[] = { "Range" };
    const size_t headerKeysToCollectChunkCount = sizeof(headerKeysToCollectChunk) / sizeof(headerKeysToCollectChunk[0]);

    bool lastChunk = (offset + readBytes == fileSize);

    HTTPClient httpsChunk;
    httpsChunk.begin(*googleWifiClient, location);
    httpsChunk.collectHeaders(headerKeysToCollectChunk, headerKeysToCollectChunkCount);
    Serial.print("Chunk-location: ");
    Serial.println(location);
    httpsChunk.addHeader("Content-Length", String(readBytes));
    httpsChunk.addHeader("Content-Range", "bytes " + String(offset) + "-" + String(offset + readBytes - 1) + (lastChunk ? "/" + String(fileSize) : "/*"));

    int chunkCode = httpsChunk.sendRequest("PUT", buffer, readBytes);  // Send chunk

    if (chunkCode != 308 && chunkCode != 200 && chunkCode != 201) {
      Serial.println("Chunk upload failed, error: " + httpsChunk.errorToString(chunkCode));

      // Read the response body for more details
      String responseBody = httpsChunk.getString();
      Serial.println("Response Body: " + responseBody);

      success = false;
    } else if (chunkCode == 308) {
      String range = httpsChunk.header("Range");
      if (range.startsWith("bytes=")) {
        range = range.substring(6);  // Remove "bytes=" part
        int dashIndex = range.indexOf('-');
        if (dashIndex != -1) {
          String lastByteStr = range.substring(dashIndex + 1);
          long lastByte = lastByteStr.toInt();
          offset = lastByte + 1;  // Set offset for next chunk
        }
      }
    }

    offset += readBytes;
  }

  file.close();
  free(buffer);

  return success;
}

bool textToSpeech(String text) {
  Serial.print("Getting Text to Speech........ ");

  HTTPClient httpClient;
  if (!httpClient.begin(*googleWifiClient, "https://texttospeech.googleapis.com/v1/text:synthesize")) {
    Serial.println("Failed to start HTTP client");
    return false;
  }

  // Use PSRAM for the JSON document
  SpiRamJsonDocument doc(8192);  // Increase size if needed
  doc["input"]["text"] = text;
  doc["voice"]["languageCode"] = "id-ID";
  doc["voice"]["name"] = "id-ID-Standard-A";
  doc["voice"]["ssmlGender"] = "FEMALE";
  doc["audioConfig"]["audioEncoding"] = "LINEAR16";
  doc["audioConfig"]["sampleRateHertz"] = 44100;  // Example sample rate, adjust as needed

  String requestBody;
  serializeJson(doc, requestBody);

  httpClient.addHeader("Content-Type", "application/json");
  httpClient.addHeader("Authorization", "Bearer " + getOAuthToken());
  int httpCode = httpClient.POST(requestBody);

  if (httpCode != 200) {
    Serial.print("Failed with status code: ");
    Serial.println(httpCode);
    Serial.println("responseString: ");
    Serial.println(httpClient.getString());
    httpClient.end();
    return false;
  }

  // Read the response in chunks
  WiFiClient* stream = httpClient.getStreamPtr();
  std::vector<uint8_t> responseData;
  while (httpClient.connected()) {
    if (!stream->available()) {
      delay(10);  // Wait for data to become available
      continue;
    }

    // Read the size of the chunk
    String line = stream->readStringUntil('\n');
    line.trim();                                 // Remove any trailing whitespace
    long size = strtol(line.c_str(), NULL, 16);  // Convert hex string to integer

    if (size <= 0) break;  // Last chunk or error

    // Read the chunk data
    while (size > 0) {
      if (stream->available()) {
        uint8_t c = stream->read();
        responseData.push_back(c);
        size--;
      } else {
        delay(10);  // Wait for more data
      }
    }

    // Read and discard the trailing newline
    stream->readStringUntil('\n');
  }
  httpClient.end();

  // Convert responseData to a char array for JSON deserialization
  char* jsonCharArray = (char*)malloc(responseData.size() + 1);  // +1 for null terminator
  if (jsonCharArray == nullptr) {
    Serial.println("Failed to allocate memory for JSON char array");
    return false;
  }

  memcpy(jsonCharArray, responseData.data(), responseData.size());
  jsonCharArray[responseData.size()] = '\0';  // Null-terminate the string

  // Save the JSON response to an SD card
  // File jsonFile = SD.open("/response.json", FILE_WRITE);
  // if (!jsonFile) {
  //   Serial.println("Failed to open file on SD card");
  //   free(jsonCharArray);  // Free the memory
  //   return;
  // }

  // jsonFile.print(jsonCharArray);
  // jsonFile.close();
  // Serial.println("Saved JSON response to /response.json");

  // Find the position of the "audioContent" field in the jsonCharArray
  char* keyStartPos = strstr(jsonCharArray, "\"audioContent\":");
  if (keyStartPos == nullptr) {
    Serial.println("Failed to find 'audioContent' field in the JSON response");
    free(jsonCharArray);
    return false;
  }

  // Move to the beginning of the audio content value, after the key's closing quote
  char* valueStartPos = strchr(keyStartPos + 15, '"');  // 15 is to skip past "\"audioContent\":"
  if (valueStartPos == nullptr) {
    Serial.println("Failed to locate the start of the audio content");
    free(jsonCharArray);
    return false;
  }
  valueStartPos += 1;  // Skip past the opening quote of the value

  // Locate the end of the audio content value (next quote after valueStartPos)
  char* valueEndPos = strchr(valueStartPos, '"');
  if (valueEndPos == nullptr) {
    Serial.println("Failed to locate the end of the audio content");
    free(jsonCharArray);
    return false;
  }

  // Temporarily replace the closing quote with a null character
  char originalChar = *valueEndPos;
  *valueEndPos = '\0';

  // Calculate the buffer size for the decoded data
  unsigned int binary_length = decode_base64_length((unsigned char*)valueStartPos);

  // Allocate memory for the binary buffer
  unsigned char* binary = (unsigned char*)ps_malloc(binary_length);
  if (!binary) {
    Serial.println("Failed to allocate memory for binary buffer");
    *valueEndPos = originalChar;  // Restore the original character
    free(jsonCharArray);
    return false;
  }

  // Decode the base64 string
  unsigned int decoded_length = decode_base64((unsigned char*)valueStartPos, binary);
  if (decoded_length == 0) {
    Serial.println("Base64 decoding failed");
    *valueEndPos = originalChar;  // Restore the original character
    free(binary);
    free(jsonCharArray);
    return false;
  }

  // Write the decoded audio data to a file
  File audioFile = SD.open(samplesGoogleFilePath, FILE_WRITE);
  if (!audioFile) {
    Serial.println("Failed to open audio file for writing");
    free(binary);
    return false;
  }

  audioFile.write(binary, decoded_length);
  audioFile.close();

  // Restore the original character and free the original JSON char array
  *valueEndPos = originalChar;
  free(jsonCharArray);

  Serial.println("Audio file saved.");

  return true;
}

String speechToText() {
  Serial.println("Getting Speech to Text........ ");

  HTTPClient httpClient;
  if (!httpClient.begin(*googleWifiClient, "https://speech.googleapis.com/v1p1beta1/speech:recognize")) {
    Serial.println("Failed to start HTTP client");
    return "";
  }

  String reqBody = "{\"config\":{\"encoding\":\"LINEAR16\",\"languageCode\":\"id-ID\"},\"audio\":{\"uri\":\"";
  reqBody += "gs://" + String(GCS_BUCKET) + "/" + String(samplesRecordedFilePath).substring(1);
  reqBody += "\"}}";

  httpClient.addHeader("Content-Type", "application/json");
  httpClient.addHeader("Authorization", "Bearer " + getOAuthToken());
  httpClient.addHeader("Content-Length", String(reqBody.length()));
  int httpCode = httpClient.sendRequest("POST", reqBody);

  String responseString = httpClient.getString();

  httpClient.end();

  // Serial.print("Status Code ");
  // Serial.println(httpCode);
  // Serial.print("Response: ");
  // Serial.println(responseString);

  if (httpCode > 0) {
    DynamicJsonDocument respDoc(4096);
    deserializeJson(respDoc, responseString);
    String content = respDoc["results"][0]["alternatives"][0]["transcript"].as<String>();
    float confidence = respDoc["results"][0]["alternatives"][0]["confidence"].as<float>();

    // Serial.print("Transcripted Audio: ");
    // Serial.println(content);
    // Serial.print("Transcription Confidence: ");
    // Serial.println(confidence);

    return content;
  }

  return "";
}

//Use Google refresh token to obtain your Oauth 2.0 token
String getOAuthToken() {
  Serial.print("Requesting Auth 2.0 Token.....");

  HTTPClient httpClient;
  httpClient.setTimeout(60000);

  httpClient.begin(*googleWifiClient, "https://oauth2.googleapis.com/token");
  httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String requestBody = "grant_type=refresh_token&response_type=token&refresh_token=" + String(REFRESHTOKENGOOGLE) + "&client_secret=" + String(CLIENTSECRETGOOGLE) + "&client_id=" + String(CLIENTIDGOOGLE);
  int httpCode = httpClient.POST(requestBody);

  String responseString = httpClient.getString();  // get the whole response at once
  DynamicJsonDocument respDoc(1024);
  deserializeJson(respDoc, responseString);  //deserialize into JSON

  String accessToken = respDoc["access_token"];  //obtain oauth2.0 token
  if (httpCode == 200 && accessToken.length() > 30) { Serial.println(" Success!"); }

  httpClient.end();
  return accessToken;  //return token
}

// ... other function implementations