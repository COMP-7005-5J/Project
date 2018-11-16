#include "Log.h"
#include "Packet.h"
#include <stdio.h>
#include <stdarg.h> // va_list, va_start, va_end

void logMessage(char *format, ...)
{
	va_list ap;

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);

    va_start(ap, format);
    vfprintf(logFile, format, ap);
    va_end(ap);
}
void logPacketType(int packetType)
{
	switch (packetType)
	{
		case (DATA):
			logMessage("DATA");
			break;
		case (EOT):
			logMessage("EOT");
			break;
		default:
			break;
	}
}