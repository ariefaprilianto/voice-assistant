#ifndef Google_Wrapper_h
#define Google_Wrapper_h

#include "global.h"
#include "config.h"

bool uploadFileToGCS(const char* fileName);
bool textToSpeech(String text);
String speechToText();
String getOAuthToken();

#endif