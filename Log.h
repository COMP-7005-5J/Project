#ifndef LOG_H_
#define LOG_H_

#pragma once

#include <stdio.h>
#include <time.h>

struct tm lastTimeLogged;
FILE *logFile;
void logMessage(int wantToDisplayTime, char *format, ...);
void logPacketType(int packetType);

#endif