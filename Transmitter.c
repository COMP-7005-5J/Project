#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#define BUFLEN 255
#define SLIDING_WINDOW_SIZE 4
#define UNINITIALISED 0 // when the packet is uninitialised
#define DATA 1
#define EOT 2
#define ACK 3

struct packet
{
	int PacketType;
	int SeqNum;
	char data[BUFLEN];
	int WindowSize;
	int AckNum;
};

int main()
{
	char buffer[BUFLEN];
	char networkIP[16], networkPort[5];
	FILE *configFile = fopen("config.txt", "r");
	FILE *logFile = fopen("log.txt", "a");
	FILE *fileToSend;
	int seqNum = 1;
	int onTheLastPacket = 0;
	int timeoutLengthMs = 1000;
	int transmitterSocket;
	struct packet packets[SLIDING_WINDOW_SIZE];
	struct packet recvPackets[SLIDING_WINDOW_SIZE];
	ssize_t numOfElementsRead = 0;
	struct sockaddr_in netEmuSvr;

	// Get the network emulator’s configurations
	fscanf(configFile, "%s %s %*s %*s", networkIP, networkPort);
	fprintf(stdout, "Got network IP and port number\n");
	fprintf(logFile, "Got network IP and port number\n");

	// Create socket
	transmitterSocket = socket(AF_INET, SOCK_DGRAM, 0);
	fprintf(stdout, "Created socket\n");
	fprintf(logFile, "Created socket\n");
	setsockopt(transmitterSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutLengthMs, sizeof(timeoutLengthMs));

	// Set up destination server
	netEmuSvr.sin_family = AF_INET;
	netEmuSvr.sin_addr.s_addr = inet_addr(networkIP);
	netEmuSvr.sin_port = htons(networkPort);

	// Open the file to send
	fprintf(stdout, "Enter name of file\n");
	fgets(buffer, sizeof(buffer), stdin);
	fileToSend = fopen(buffer, "r");
	fprintf(stdout, "Opened file to send\n");
	fprintf(logFile, "Opened file to send\n");
	memset(buffer, 0, BUFLEN); // Reset buffer

	// Loop forever until there’s nothing else to do
	while (1)
	{
		// Create the DATA packets
		for (int i = 0; i < (SLIDING_WINDOW_SIZE - 1); i++, seqNum++)
		{
			if ((packets[i].PacketType == UNINITIALISED) && (onTheLastPacket == 0))
			{
				packets[i].PacketType = DATA;
				packets[i].SeqNum = seqNum;
				packets[i].WindowSize = fread(packets[i].data, sizeof(buffer), sizeof(char), fileToSend);
				packets[i].AckNum = packets[i].SeqNum * packets[i].WindowSize + 1;
		
				// If the window size is less than BUFFER length, then we have the last bits of data
				if (packets[i].WindowSize < BUFLEN)
				{
					onTheLastPacket = 1;
					break;
				}
			}
		}
		
		// Create the EOT packet
		packets[SLIDING_WINDOW_SIZE - 1].PacketType = EOT;
		packets[SLIDING_WINDOW_SIZE - 1].SeqNum = seqNum++;

		// Send the packets
		for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
		{
			if (packets[i].PacketType != UNINITIALISED)
				sendto(transmitterSocket, &packets[i], sizeof(struct packet), 0, &netEmuSvr, sizeof(netEmuSvr));
			if (packets[i].WindowSize < BUFLEN)
				break;
		}
		
		// Receive the ACKs
		for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
		{
			numOfElementsRead = recvfrom(transmitterSocket, &recvPackets[i], sizeof(recvPackets[i]), 0, &netEmuSvr, sizeof(netEmuSvr));
			if (recvPackets[i].PacketType == EOT)
				break;
			else
				packets[recvPackets[i].AckNum].PacketType = UNINITIALISED;
		}
		
		// If we're in the last phase...
		if (onTheLastPacket)
		{
			// Make sure all the packets have been acknowledged
			for (int i = 0; i < (SLIDING_WINDOW_SIZE - 1); i++, seqNum++)
			{
				if (packets[i].PacketType != UNINITIALISED)
				{
					onTheLastPacket = 0;
					break;
				}
			}
			
			if (onTheLastPacket)
			{
				break;
			}
		}
	}

	fclose(configFile);
}
