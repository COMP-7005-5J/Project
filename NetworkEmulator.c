#include <stdio.h> // FILE
#include <stdlib.h> // fprintf()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // getsockname()
#include <unistd.h> // sleep()
#include <errno.h> // errno
#include <string.h>
#include <arpa/inet.h> // ntoa()

#define BUFLEN 255
#define DATA 1
#define EOT 2
#define ACK 3

int bitErrorRate;
int emulatorSocket;
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
	FILE *logFile = fopen("./logEmulator.txt", "a");
	if (sendto(emulatorSocket, &pkt, sizeof(pkt), 0, (struct sockaddr *)DestSvr, sizeof(*DestSvr)) < 0)
	{
		fprintf(stdout, "ERROR: Couldn't send pkt. %s\n", strerror(errno));
		fprintf(logFile, "ERROR: Couldn't send pkt. %s\n", strerror(errno));
	}
	else
	{
		if (pkt.PacketType == DATA)
		{
			fprintf(stdout, "Forwarded Data Packet[%d] to %s:%d\n", pkt.SeqNum, inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port));
			fprintf(logFile, "Forwarded Data Packet[%d]\n", pkt.SeqNum);
		}
		else if (pkt.PacketType == EOT)
		{
			fprintf(stdout, "Forwarded EOT Packet[%d] to %s:%d\n", pkt.SeqNum, inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port));
			fprintf(logFile, "Forwarded Data Packet[%d]\n", pkt.SeqNum);
		}
		else if (pkt.PacketType == ACK)
		{
			fprintf(stdout, "Forwarded ACK Packet[%d] to %s:%d\n", pkt.AckNum, inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port));
			fprintf(logFile, "Forwarded ACK Packet[%d] to %s:%d\n", pkt.AckNum, inet_ntoa(DestSvr->sin_addr), ntohs(DestSvr->sin_port));
		}
	}
}

int main()
{
	char buffer[BUFLEN];
	char networkIP[16], networkPort[6], receiverIP[16], receiverPort[6];
	FILE *configFile = fopen("./config.txt", "r");
	FILE *logFile = fopen("./logEmulator.txt", "w");
	int avgDelay;
	int eotRecvd = 0;
	int eotSent = 0;
	int pktsDelayed = 0;
	int recvfromResult = 0;
	pthread_t thread;
	socklen_t fromLen;
	socklen_t len;
	struct packet recvPacket;
	struct packet pkt;
	struct sockaddr_in recSvr;
	struct sockaddr_in netEmuSvr;
	struct sockaddr_in fromAddr;
	
	// Get the network emulator’s configurations
	fscanf(configFile, "%s %s %s %s", networkIP, networkPort, receiverIP, receiverPort);
	fprintf(stdout, "Loaded configurations\n");
	fprintf(logFile, "Loaded configurations\n");

	// Create socket
	emulatorSocket = socket(AF_INET, SOCK_DGRAM, 0);
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
	
	while (1)
	{
		while (eotRecvd == 0)
		{
			fromLen = sizeof(fromAddr);
			if (recvfrom(emulatorSocket, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr*)&fromAddr, &fromLen) < 0)
			{
				fprintf(stdout, "Error: Couldn't receive packets. %s\n", strerror(errno));
				fprintf(logFile, "Error: Couldn't receive packets. %s\n", strerror(errno));
				break;
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
		
		DestSvr = &fromAddr;
		pktsDelayed = 0;
		
		while (eotSent == 0)
		{
			if (recvfrom(emulatorSocket, &recvPacket, sizeof(recvPacket), 0, NULL, NULL) < 0)
			{
				fprintf(stdout, "Error: Couldn't receive packets. %s\n", strerror(errno));
				fprintf(logFile, "Error: Couldn't receive packets. %s\n", strerror(errno));
				exit(0);
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
	}
	fclose(configFile);
	close(emulatorSocket);
	exit(0);
}