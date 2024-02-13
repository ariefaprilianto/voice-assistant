#include "processing.h"
#include "global.h"
#include "config.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include "SpiRamAllocator.h"

String getAnswer(String question){
  Serial.println("Sending Open AI Call.......... ");

  // OpenAI API details
  const char* serverAddress = "api.openai.com";
  int port = 443;
  const char* apiPath = "/v1/chat/completions";

  // Initialize the Wi-Fi SSL client and HTTP client
  WiFiClientSecure client;
  client.setInsecure();
  HttpClient httpClient(client, serverAddress, port);

  // Prepare JSON payload using SpiRamJsonDocument
  SpiRamJsonDocument doc(2048);
  doc["model"] = "gpt-3.5-turbo";
  doc["max_tokens"] = 100;
  JsonArray messages = doc.createNestedArray("messages");
  JsonObject message = messages.createNestedObject();
  message["role"] = "user";
  message["content"] = question;

  String requestBody;
  serializeJson(doc, requestBody);

  // Debugging: Print the request body
  Serial.println("Request Body:");
  Serial.println(requestBody);

  // Start request call
  httpClient.beginRequest();
  httpClient.post(apiPath);

  // Add headers
  httpClient.sendHeader("Authorization", "Bearer " + String(APIKEYOPENAI));
  httpClient.sendHeader("Content-Type", "application/json");
  httpClient.sendHeader("Content-Length", requestBody.length());

  httpClient.beginBody();
  httpClient.print(requestBody);

  httpClient.endRequest();

  // Read the response status and body
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  // Debugging: Print the status code and response body
  Serial.print("Status Code: ");
  Serial.println(statusCode);
  Serial.println("Response Body:");
  Serial.println(response);

  // Handle rate limiting
  if (statusCode != 200) {
    Serial.print("Status Code: ");
    Serial.println(statusCode);
    Serial.println("Response Body:");
    Serial.println(response);
    return "";
  }

  // Parse the response JSON
  SpiRamJsonDocument responseDoc(2048);
  DeserializationError error = deserializeJson(responseDoc, response);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return "";
  }

  // Extract the content field
  String content = responseDoc["choices"][0]["message"]["content"];

  Serial.print("Content: ");
  Serial.println(content);
  return content; // Return the OpenAI response to question asked
}