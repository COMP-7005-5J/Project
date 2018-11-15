#include <stdio.h> // FILE
#include <stdlib.h> // fprintf()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // getsockname(), setsockopt()
#include <unistd.h> // sleep()
#include <errno.h> // errno
#include <string.h>
#include <arpa/inet.h> // ntoa()
#include <sys/time.h> // struct timeval

#define BUFLEN 255
#define DATA 1
#define EOT 2
#define ACK 3

FILE *logFile;
int bitErrorRate;
int emulatorSocket;
int directionToRec = 1;
struct sockaddr_in *DestSvr;

struct packet
{
	int PacketType;
	int SeqNum;
	char data[BUFLEN];
	int WindowSize;
	int AckNum;
};

void forward(struct packet pkt)
{
	//FILE *logFile = fopen("./logEmulator.txt", "a");
	char *type = malloc(4 * sizeof(*type));
	int repNum;
	
	switch(pkt.PacketType)
	{
		case (DATA):
			strcpy(type, "DATA");
			repNum = pkt.SeqNum;
			break;
		case (EOT):
			strcpy(type, "EOT");
			if (directionToRec)
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
	
	if ((rand() % 100) > bitErrorRate)
	{
		if (sendto(emulatorSocket, &pkt, sizeof(pkt), 0, (struct sockaddr *)DestSvr, sizeof(*DestSvr)) < 0)
		{
			fprintf(stdout, "ERROR: Couldn't send pkt. %s\n", strerror(errno));
			fprintf(logFile, "ERROR: Couldn't send pkt. %s\n", strerror(errno));
		}
		else
		{
			if (directionToRec)
			{
				fprintf(stdout, "%s[%d] >>> %s:%d\n", type, repNum, inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port));
				fprintf(logFile, "%s[%d] >>> %s:%d\n", type, repNum, inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port));
			}
			else
			{
				fprintf(stdout, "%s:%d <<< %s[%d]\n", inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port), type, repNum);
				fprintf(logFile, "%s:%d <<< %s[%d]\n", inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port), type, repNum);
			}
		}
	}
	else
	{
		fprintf(stdout, "Dropped %s[%d]\n", type, repNum);
		fprintf(logFile, "Dropped %s[%d]\n", type, repNum);
	}
	
	//fclose(logFile);
	free(type);
}

int main()
{
	char buffer[BUFLEN];
	char networkIP[16], networkPort[6], receiverIP[16], receiverPort[6];
	FILE *configFile = fopen("./config.txt", "r");
	logFile = fopen("./logEmulator.txt", "w+");
	int avgDelay;
	int eotRecvd = 0;
	int eotSent = 0;
	int pktsDelayed = 0;
	socklen_t fromLen;
	socklen_t len;
	struct packet recvPacket;
	struct packet pkt;
	struct sockaddr_in recSvr;
	struct sockaddr_in netEmuSvr;
	struct sockaddr_in fromAddr;
	struct timeval timeout = { .tv_sec = 4, .tv_usec = 0};
	
	// Get the network emulator’s configurations
	fscanf(configFile, "%s %s %s %s", networkIP, networkPort, receiverIP, receiverPort);
	fprintf(stdout, "Loaded configurations\n");
	fprintf(logFile, "Loaded configurations\n");

	// Create socket
	emulatorSocket = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(emulatorSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	fprintf(stdout, "Created socket\n");
	fprintf(logFile, "Created socket\n");
	
	// Set up network emulator server
	bzero((char*)&netEmuSvr,sizeof(struct sockaddr_in));
	netEmuSvr.sin_family = AF_INET;
	netEmuSvr.sin_addr.s_addr = inet_addr(networkIP);
	netEmuSvr.sin_port = htons(atoi(networkPort));
	fprintf(stdout, "Created network emulator server\n");
	fprintf(stdout, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	fprintf(stdout, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	fprintf(logFile, "Created server\n");
	fprintf(logFile, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	fprintf(logFile, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	
	// Bind socket
	len = sizeof(netEmuSvr);
	if (bind(emulatorSocket, (struct sockaddr *) &netEmuSvr, len) < 0)
	{
		fprintf(stdout, "ERROR: Couldn't bind socket. %s\n", strerror(errno));
		fprintf(logFile, "ERROR: Couldn't bind socket. %s\n", strerror(errno));
		exit(0);
	}
	fprintf(stdout, "Binded socket\n");
	fprintf(logFile, "Binded socket\n");
	
	// Set up destination server
	recSvr.sin_family = AF_INET;
	inet_aton(receiverIP, &recSvr.sin_addr);
	recSvr.sin_port = htons(atoi(receiverPort));
	DestSvr = &recSvr;
	fprintf(stdout, "Created destination server\n");
	fprintf(stdout, "\tAddress: %s\n", inet_ntoa(DestSvr->sin_addr));
	fprintf(stdout, "\tPort: %d\n", ntohs(DestSvr->sin_port));
	fprintf(logFile, "Created destination server\n");
	fprintf(logFile, "\tAddress: %s\n", inet_ntoa(DestSvr->sin_addr));
	fprintf(logFile, "\tPort: %d\n", ntohs(DestSvr->sin_port));
	
	// Get the BER
	fprintf(stdout, "Enter your desired Bit Error Rate percentage (int)\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	bitErrorRate = atoi(buffer);
	memset(buffer, 0, sizeof(buffer)); // Reset buffer
	fprintf(stdout, "BER: %d\n", bitErrorRate);
	fprintf(logFile, "BER: %d\n", bitErrorRate);
	
	// Get the Avg Delay
	fprintf(stdout, "Enter your desired delay when a packet is received and sent (seconds)\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	avgDelay = atoi(buffer);
	fprintf(stdout, "Avg Delay: %d\n", avgDelay);
	fprintf(logFile, "Avg Delay: %d\n", avgDelay);
	
	fprintf(stdout, "STARTING SERVICE\n\n");
	fprintf(logFile, "STARTING SERVICE\n\n");
	fclose(logFile);
	while (1)
	{
		logFile = fopen("./logEmulator.txt", "a");
		fprintf(stdout, "\n");
		fprintf(logFile, "\n");
		fprintf(stdout, "Waiting for DATA...\n");
		fprintf(logFile, "Waiting for DATA...\n");
		while (eotRecvd == 0)
		{
			fromLen = sizeof(fromAddr);
			if (recvfrom(emulatorSocket, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr*)&fromAddr, &fromLen) < 0)
			{
				continue;
			}
			else
			{
				if (recvPacket.PacketType == EOT)
				{
					eotRecvd = 1;
				}
				
				pkt = recvPacket;
				if (pktsDelayed == 0)
				{
					sleep(avgDelay);
					pktsDelayed = 1;
				}
				forward(pkt);
			}
		}
		fprintf(stdout, "\n");
		fprintf(logFile, "\n");
		
		DestSvr = &fromAddr;
		pktsDelayed = 0;
		directionToRec = 0;
		
		fprintf(stdout, "Waiting for ACKs...\n");
		fprintf(logFile, "Waiting for ACKs...\n");
		while (eotSent == 0)
		{
			if (recvfrom(emulatorSocket, &recvPacket, sizeof(recvPacket), 0, NULL, NULL) < 0)
			{
				// Timeout occurred
				fprintf(stdout, "===Timeout occurred===\n");
				fprintf(logFile, "===Timeout occurred===\n");
				break;
			}
			else
			{
				if (recvPacket.PacketType == EOT)
				{
					eotSent = 1;
				}
				
				pkt = recvPacket;
				if (pktsDelayed == 0)
				{
					sleep(avgDelay);
					pktsDelayed = 1;
				}
				forward(pkt);
			}
		}
		
		// Reset variables
		eotRecvd = 0;
		eotSent = 0;
		DestSvr = &recSvr;
		pktsDelayed = 0;
		directionToRec = 1;
		fclose(logFile);
	}
	fclose(configFile);
	close(emulatorSocket);
	exit(0);
}