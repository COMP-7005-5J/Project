#ifndef LOG_H_
#define LOG_H_

#pragma once

#include <stdio.h>

FILE *logFile;
void logMessage(char *format, ...);
void logPacketType(int packetType);

#endif