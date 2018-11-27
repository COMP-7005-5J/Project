#include "../Log.h"
#include "../Packet.h"

#include <stdio.h> // FILE
#include <stdlib.h> // fprintf()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // getsockname(), setsockopt()
#include <unistd.h> // sleep()
#include <errno.h> // errno
#include <string.h>
#include <arpa/inet.h> // ntoa()
#include <sys/time.h> // struct timeval

//
#define FROM_RECEIVER 0;
#define FROM_TRANSMITTER 1;

int bitErrorRate;
int emulatorSocket;
int directionToRec = FROM_TRANSMITTER;
int eotSent = 0;
struct sockaddr_in *DestSvr;

// forwards the packet to a destination determined by directionToRec.
void forward(struct packet pkt)
{
	//FILE *logFile = fopen("./logEmulator.txt", "a");
	char *type = malloc(4 * sizeof(*type));
	int repNum;
	int sentMsgLen;

	switch(pkt.PacketType)
	{
		case (DATA):
			strcpy(type, "DATA");
			repNum = pkt.SeqNum;
			break;
		case (EOT):
			strcpy(type, "EOT");
			eotSent = 1;
			if (directionToRec == FROM_TRANSMITTER)
			{
				repNum = pkt.SeqNum;
			}
			else
			{
				repNum = pkt.AckNum;
			}
			break;
		default:
			strcpy(type, "ACK");
			repNum = pkt.AckNum;
			break;
	}
	
	// generate a random number to see if the packet should be sent
	if ((rand() % 100) > bitErrorRate)
	{
		sentMsgLen = sendto(emulatorSocket, &pkt, sizeof(pkt), 0, (struct sockaddr *)DestSvr, sizeof(*DestSvr));
		if (sentMsgLen == -1)
		{
			logMessage(0, "ERROR: Couldn't send pkt. %s\n", strerror(errno));
		}
		else
		{
			if (directionToRec == FROM_TRANSMITTER)
			{
				logMessage(0, "%s[%d] >>> %s:%d\n", type, repNum, inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port));
			}
			else
			{
				logMessage(0, "%s:%d <<< %s[%d]\n", inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port), type, repNum);
			}
		}
	}
	else
	{
		if (eotSent == 1)
		{
			eotSent = 0;
		}
		logMessage(1, "Dropped %s[%d]\n", type, repNum);
	}
	
	//fclose(logFile);
	free(type);
}

int main()
{
	char buffer[BUFLEN];
	char networkIP[16], networkPort[6], receiverIP[16], receiverPort[6];
	FILE *configFile = fopen("../config.txt", "r");
	logFile = fopen("./logEmulator.txt", "w+");
	int avgDelay;
	int receivedMsgLen;
	int pktsDelayed = 0;
	socklen_t fromLen;
	socklen_t len;
	struct packet recvPacket;
	struct packet pkt;
	struct sockaddr_in recSvr;
	struct sockaddr_in netEmuSvr;
	struct sockaddr_in fromAddr;
	
	// Get the network emulator’s configurations
	fscanf(configFile, "%s %s %s %s", networkIP, networkPort, receiverIP, receiverPort);
	logMessage(1, "Loaded configurations\n");

	// Create socket
	emulatorSocket = socket(AF_INET, SOCK_DGRAM, 0);
	logMessage(1, "Created socket\n");
	
	// Set up network emulator server
	bzero((char*)&netEmuSvr,sizeof(struct sockaddr_in));
	netEmuSvr.sin_family = AF_INET;
	netEmuSvr.sin_addr.s_addr = inet_addr(networkIP);
	netEmuSvr.sin_port = htons(atoi(networkPort));
	logMessage(1, "Created network emulator server\n");
	logMessage(0, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	logMessage(0, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	
	// Bind socket
	len = sizeof(netEmuSvr);
	if (bind(emulatorSocket, (struct sockaddr *) &netEmuSvr, len) < 0)
	{
		logMessage(1, "ERROR: Couldn't bind socket. %s\n", strerror(errno));
		exit(0);
	}
	logMessage(1, "Binded socket\n");
	
	// Set up destination server
	recSvr.sin_family = AF_INET;
	inet_aton(receiverIP, &recSvr.sin_addr);
	recSvr.sin_port = htons(atoi(receiverPort));
	DestSvr = &recSvr;
	logMessage(1, "Created destination server\n");
	logMessage(0, "\tAddress: %s\n", inet_ntoa(DestSvr->sin_addr));
	logMessage(0, "\tPort: %d\n", ntohs(DestSvr->sin_port));
	
	// Get the BER
	fprintf(stdout, "Enter your desired Bit Error Rate percentage (int)\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	bitErrorRate = atoi(buffer);
	memset(buffer, 0, sizeof(buffer)); // Reset buffer
	logMessage(1, "BER: %d\n", bitErrorRate);
	
	// Get the Avg Delay
	fprintf(stdout, "Enter your desired delay when a packet is received and sent (seconds)\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	avgDelay = atoi(buffer);
	logMessage(1, "Avg Delay: %d\n", avgDelay);
	
	logMessage(1, "STARTING SERVICE\n\n");
	fclose(logFile);
	while (1)
	{
		logFile = fopen("./logEmulator.txt", "a");
		logMessage(0, "\n");
		logMessage(1, "Waiting for DATA...\n");
		while (eotSent == 0)
		{
			fromLen = sizeof(fromAddr);
			receivedMsgLen = recvfrom(emulatorSocket, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr *)&fromAddr, &fromLen);
			if (receivedMsgLen == -1)
			{
				continue;
			}
			else
			{
				pkt = recvPacket;
				if (pktsDelayed == 0)
				{
					sleep(avgDelay);
					pktsDelayed = 1;
				}
				forward(pkt);
			}
		}
		logMessage(0, "\n");
		
		DestSvr = &fromAddr;
		pktsDelayed = 0;
		directionToRec = FROM_RECEIVER;
		
		logMessage(1, "Waiting for ACKs...\n");
		while (1)
		{
			receivedMsgLen = recvfrom(emulatorSocket, &recvPacket, sizeof(recvPacket), 0, NULL, NULL);
			if (receivedMsgLen == -1)
			{
				// Timeout occurred
				logMessage(1, "===Timeout occurred===\n");
				break;
			}
			else
			{
				pkt = recvPacket;
				if (pktsDelayed == 0)
				{
					sleep(avgDelay);
					pktsDelayed = 1;
				}
				forward(pkt);
				
				if (recvPacket.PacketType == EOT)
				{
					break;
				}
			}
		}
		
		// Reset variables
		eotSent = 0;
		DestSvr = &recSvr;
		pktsDelayed = 0;
		directionToRec = FROM_TRANSMITTER;
		fclose(logFile);
	}
	fclose(configFile);
	close(emulatorSocket);
	return 0;
}