#include "../Log.h"
#include "../Packet.h"

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h> // ntoa()
#include <errno.h> // errno
#include <sys/time.h> // struct timeval
#include <netdb.h> // gethostbyname

#define SLIDING_WINDOW_SIZE 4

FILE *logFile;

int main()
{	
	char buffer[BUFLEN];
	char networkIP[16], networkPort[6];
	FILE *configFile = fopen("../config.txt", "r");
	logFile = fopen("./logTransmitter.txt", "w+");
	FILE *fileToSend;
	int allPacketsAckd = 1;
	int eotRecvd = 0;
	int seqNum = 1;
	int onTheLastPacket = 0;
	int port = 8080;
	int timeoutOccurred = 0;
	int windowSlideDistance = SLIDING_WINDOW_SIZE;
	struct timeval timeout = { .tv_sec = 7, .tv_usec = 0};
	int transmitterSocket;
	socklen_t len;
	struct hostent *hp;
	struct packet packets[SLIDING_WINDOW_SIZE];
	struct packet recvPacket;
	struct sockaddr_in netEmuSvr;
	struct sockaddr_in transmitterSvr;
	
	// Set up packets
	for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
	{
		packets[i].PacketType = UNINITIALISED;
	}
	
	// Get the network emulator’s configurations
	fscanf(configFile, "%s %s %*s %*s", networkIP, networkPort);
	logMessage(1, "Loaded configurations\n");

	// Create socket
	transmitterSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (transmitterSocket > 0)
	{
		setsockopt(transmitterSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		logMessage(1, "Created socket\n");
	}
	else
	{
		logMessage(1, "ERROR: Creating socket. %s\n", strerror(errno));
		exit(0);
	}
	
	// Set up transmitter server
	bzero((char*)&netEmuSvr,sizeof(struct sockaddr_in));
	transmitterSvr.sin_family = AF_INET;
	
	// Get the host
	fprintf(stdout, "Enter your IP address (host)\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	if ((hp = gethostbyname(buffer)) == NULL)
	{
		fprintf(stderr, "Unknown server address\n");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&transmitterSvr.sin_addr, hp->h_length);
	memset(buffer, 0, BUFLEN); // Reset buffer
	
	//Get the port
	fprintf(stdout, "Enter your preferred host (if you enter nothing then the default port, %d, will be used)\n", port);
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	if (strcmp(buffer, "") != 0)
	{
		port = atoi(buffer);
	}
	transmitterSvr.sin_port = htons(port);
	
	// Log resulting server
	logMessage(1, "Created transmitter server\n");
	logMessage(0, "\tAddress: %s\n", inet_ntoa(transmitterSvr.sin_addr));
	logMessage(0, "\tPort: %d\n", ntohs(transmitterSvr.sin_port));
	
	// Bind socket
	len = sizeof(transmitterSvr);
	if (bind(transmitterSocket, (struct sockaddr *) &transmitterSvr, len) < 0)
	{
		logMessage(1, "ERROR: Couldn't bind socket. %s\n", strerror(errno));
		exit(0);
	}
	logMessage(1, "Binded socket\n");

	// Set up network emulator server
	netEmuSvr.sin_family = AF_INET;
	netEmuSvr.sin_addr.s_addr = inet_addr(networkIP);
	netEmuSvr.sin_port = htons(atoi(networkPort));
	logMessage(1, "Created network emulator server\n");
	logMessage(0, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	logMessage(0, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	
	// Open the file to send
	fprintf(stdout, "Enter name of file\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	fileToSend = fopen(buffer, "r");
	logMessage(1, "Opened file: %s\n", buffer);
	memset(buffer, 0, BUFLEN); // Reset buffer
	
	len = sizeof(netEmuSvr);
	
	logMessage(1, "STARTING SERVICE\n\n");
	fclose(logFile);
	// Loop forever until there’s nothing else to do
	while (1)
	{
		logFile = fopen("./logTransmitter.txt", "a");
		logMessage(0, "\n");
		// Create the DATA packets
		for (int i = (SLIDING_WINDOW_SIZE - windowSlideDistance); i < SLIDING_WINDOW_SIZE; i++)
		{
			if ((packets[i].PacketType == UNINITIALISED) && (onTheLastPacket == 0))
			{
				packets[i].SeqNum = seqNum;
				packets[i].WindowSize = fread(packets[i].data, sizeof(char), BUFLEN, fileToSend);
				packets[i].AckNum = (packets[i].SeqNum * BUFLEN) - (BUFLEN - packets[i].WindowSize) + 1;
				if ((i == (SLIDING_WINDOW_SIZE - 1)) || ((BUFLEN - packets[i].WindowSize) > 0))
				{
					packets[i].PacketType = EOT;
				}
				else
				{
					packets[i].PacketType = DATA;
				}
				
				logMessage(1, "Created ");
				logPacketType(packets[i].PacketType);
				logMessage(0, "[%d], LEN: %d, ACK: %d\n", packets[i].SeqNum, packets[i].WindowSize, packets[i].AckNum);
				// If the window size is less than BUFFER length, then we have the last bits of data
				if (packets[i].WindowSize < BUFLEN)
				{
					onTheLastPacket = 1;
					seqNum++;
					break;
				}
				seqNum++;
			}
		}
			
		// In the event that a former packet[0] needs to be resent, we need to manually make it type EOT
		for (int i = (SLIDING_WINDOW_SIZE-1); i >= 0; i--)
		{
			if (packets[i].PacketType != UNINITIALISED)
		 	{
				packets[i].PacketType = EOT;
				break;
			}
		}
		
		logMessage(0, "\n");
		logMessage(1, "Window After Creating Packets { ");
		for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
		{
			if (packets[i].PacketType != UNINITIALISED)
			{
				logPacketType(packets[i].PacketType);
				logMessage(0, "[%d]", packets[i].SeqNum);
			}
			else
			{
				logMessage(0, "-%d-", packets[i].SeqNum);
			}
			if (i < (SLIDING_WINDOW_SIZE-1))
			{
				logMessage(0, ", ");
			}
		}
		logMessage(0, " }\n");

		// Send the packets
		for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
		{
			if (packets[i].PacketType != UNINITIALISED)
			{
				if (sendto(transmitterSocket, &packets[i], sizeof(struct packet), 0, (struct sockaddr*)&netEmuSvr, sizeof(netEmuSvr)) < 0)
				{
					logMessage(1, "ERROR: Couldn't send packets. %s\n", strerror(errno));
				}
				else
				{
					if (timeoutOccurred)
					{
						logMessage(1, "Resent ");
					}
					else
					{
						logMessage(1, "Sent ");
					}
					
					
					logPacketType(packets[i].PacketType);
					logMessage(0, "[%d]\n", packets[i].SeqNum);
				}
			}
		}
		if (timeoutOccurred)
		{
			timeoutOccurred = 0;
		}
		
		logMessage(0, "\n");
		
		// Receive the ACKs
		while (eotRecvd == 0)
		{
			if (recvfrom(transmitterSocket, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr*)&netEmuSvr, &len) < 0)
			{
				// Timeout occurred
				logMessage(1, "===Timeout occurred===\n");
				timeoutOccurred = 1;
				break;
			}
			else
			{
				if (recvPacket.PacketType == EOT)
				{
					eotRecvd = 1;
				}
				
				for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
				{
					if (recvPacket.AckNum == packets[i].SeqNum)
					{
						packets[i].PacketType = UNINITIALISED;
						memset(packets[i].data, 0, BUFLEN); // Reset buffer
						logMessage(1, "Received ACK[%d]\n", recvPacket.AckNum);
					}
				}
			}
		}
		
		logMessage(0, "\n");
		
		// Check to see if there are packets that didn't receive an ACK
		windowSlideDistance = SLIDING_WINDOW_SIZE;
		for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
		{
			if (packets[i].PacketType != UNINITIALISED)
			{
				windowSlideDistance = i;
				break;
			}
		}
		
		// Shift packets that weren't ACK'd to the left
		if ((windowSlideDistance > 0) && (windowSlideDistance < SLIDING_WINDOW_SIZE))
		{
			for (int i = windowSlideDistance; i < SLIDING_WINDOW_SIZE; i++)
			{
				if (packets[i].PacketType != UNINITIALISED)
				{
					packets[i-windowSlideDistance] = packets[i];
					packets[i-windowSlideDistance].PacketType = DATA;
					packets[i].PacketType = UNINITIALISED;
					packets[i].SeqNum = packets[i].SeqNum + i;
					memset(packets[i].data, 0, BUFLEN); // Reset buffer
				}
			}
		}
		
		logMessage(1, "Window After ACKs { ");
		for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
		{
			if (packets[i].PacketType != UNINITIALISED)
			{
				logPacketType(packets[i].PacketType);
				logMessage(0, "[%d]", packets[i].SeqNum);
			}
			else
			{
				logMessage(0, "-%d-", packets[i].SeqNum);
			}
			if (i < (SLIDING_WINDOW_SIZE-1))
			{
				logMessage(0, ", ");
			}
		}
		logMessage(0, " }\n");
		
		// If we're in the last phase...
		if (onTheLastPacket)
		{
			// Make sure all the packets have been acknowledged
			allPacketsAckd = 1;
			for (int i = 0; i < (SLIDING_WINDOW_SIZE - 1); i++, seqNum++)
			{
				if (packets[i].PacketType != UNINITIALISED)
				{
					allPacketsAckd = 0;
					break;
				}
			}
			
			if (allPacketsAckd)
			{
				logMessage(1, "END OF SERVICE\n");
				break;
			}
		}
		
		// Reset variables
		eotRecvd = 0;
		fclose(logFile);
	}

	close(transmitterSocket);
	fclose(configFile);
	fclose(logFile);
	return 0;
}
