#include "Log.h"
#include "Packet.h"
#include <stdarg.h> // va_list, va_start, va_end

void logMessage(int wantToDisplayTime, char *format, ...)
{
	if (wantToDisplayTime)
	{
		char buff[100];
    	time_t now = time (0);
    	struct tm *nowFormatted = localtime (&now);
    	if ((nowFormatted->tm_sec != lastTimeLogged.tm_sec) || (nowFormatted->tm_min != lastTimeLogged.tm_min) || (nowFormatted->tm_hour != lastTimeLogged.tm_hour))
    	{
    		strftime(buff, 100, "%H:%M:%S", nowFormatted);
    		fprintf(stdout, "%s: ", buff);
    		fprintf(logFile, "%s: ", buff);
    		lastTimeLogged = *nowFormatted;
    	}
	}
    
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
			logMessage(0, "DATA");
			break;
		case (EOT):
			logMessage(0, "EOT");
			break;
		default:
			break;
	}
}