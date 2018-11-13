#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h> // ntoa()
#include <errno.h> // errno

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
	char networkIP[16], networkPort[6];
	FILE *configFile = fopen("./config.txt", "r");
	FILE *logFile = fopen("./logTransmitter.txt", "w");
	FILE *fileToSend;
	int allPacketsAckd = 1;
	int seqNum = 1;
	int onTheLastPacket = 0;
	int timeoutLengthMs = 1000;
	int transmitterSocket;
	socklen_t len;
	struct packet packets[SLIDING_WINDOW_SIZE];
	struct packet recvPackets[SLIDING_WINDOW_SIZE];
	ssize_t numOfElementsRead = 0;
	struct sockaddr_in netEmuSvr;
	struct sockaddr_in transmitterSvr;

	// Get the network emulator’s configurations
	fscanf(configFile, "%s %s %*s %*s", networkIP, networkPort);
	fprintf(stdout, "Loaded configurations\n");
	fprintf(logFile, "Loaded configurations\n");

	// Create socket
	transmitterSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (transmitterSocket > 0)
	{
		setsockopt(transmitterSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutLengthMs, sizeof(timeoutLengthMs));
		fprintf(stdout, "Created socket\n");
		fprintf(logFile, "Created socket\n");
	}
	else
	{
		fprintf(stdout, "ERROR: Creating socket. %s\n", strerror(errno));
		fprintf(logFile, "ERROR: Creating socket. %s\n", strerror(errno));
		exit(0);
	}
	
	
	// Set up transmitter server
	bzero((char*)&netEmuSvr,sizeof(struct sockaddr_in));
	transmitterSvr.sin_family = AF_INET;
	transmitterSvr.sin_addr.s_addr = inet_addr("127.0.0.1");
	transmitterSvr.sin_port = htons(8080);
	fprintf(stdout, "Created transmitter server\n");
	fprintf(stdout, "\tAddress: %s\n", inet_ntoa(transmitterSvr.sin_addr));
	fprintf(stdout, "\tPort: %d\n", ntohs(transmitterSvr.sin_port));
	fprintf(logFile, "Created server\n");
	fprintf(logFile, "\tAddress: %s\n", inet_ntoa(transmitterSvr.sin_addr));
	fprintf(logFile, "\tPort: %d\n", ntohs(transmitterSvr.sin_port));
	
	// Bind socket
	len = sizeof(transmitterSvr);
	if (bind(transmitterSocket, (struct sockaddr *) &transmitterSvr, len) < 0)
	{
		fprintf(stdout, "ERROR: Couldn't bind socket. %s\n", strerror(errno));
		fprintf(logFile, "ERROR: Couldn't bind socket. %s\n", strerror(errno));
		exit(0);
	}
	fprintf(stdout, "Binded socket\n");
	fprintf(logFile, "Binded socket\n");

	// Set up destination server
	netEmuSvr.sin_family = AF_INET;
	netEmuSvr.sin_addr.s_addr = inet_addr(networkIP);
	netEmuSvr.sin_port = htons(atoi(networkPort));
	fprintf(stdout, "Created network emulator server\n");
	fprintf(stdout, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	fprintf(stdout, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	fprintf(logFile, "Created server\n");
	fprintf(logFile, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	fprintf(logFile, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	
	// Open the file to send
	fprintf(stdout, "Enter name of file\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	fileToSend = fopen(buffer, "r");
	fprintf(stdout, "Opened file: %s\n", buffer);
	fprintf(logFile, "Opened file: %s\n", buffer);
	memset(buffer, 0, BUFLEN); // Reset buffer

	// Loop forever until there’s nothing else to do
	while (1)
	{
		allPacketsAckd = 1;
		
		// Create the DATA packets
		for (int i = 0; i < (SLIDING_WINDOW_SIZE - 1); i++, seqNum++)
		{
			if ((packets[i].PacketType == UNINITIALISED) && (onTheLastPacket == 0))
			{
				packets[i].PacketType = DATA;
				packets[i].SeqNum = seqNum;
				packets[i].WindowSize = fread(packets[i].data, sizeof(char), BUFLEN, fileToSend);
				packets[i].AckNum = packets[i].SeqNum * packets[i].WindowSize + 1;
		
				fprintf(stdout, "Packet[%d]\n\tPacketType: %d\n\tSeqNum: %d\n\tWindowSize: %d\n\tAckNum: %d\n", i, packets[i].PacketType, packets[i].SeqNum, packets[i].WindowSize, packets[i].AckNum);
				fprintf(logFile, "Packet[%d]\n\tPacketType: %d\n\tSeqNum: %d\n\tWindowSize: %d\n\tAckNum: %d\n", i, packets[i].PacketType, packets[i].SeqNum, packets[i].WindowSize, packets[i].AckNum);
				
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
		fprintf(stdout, "Packet[%d]\n\tPacketType: %d\n\tSeqNum: %d\n", (SLIDING_WINDOW_SIZE - 1), packets[SLIDING_WINDOW_SIZE - 1].PacketType, packets[SLIDING_WINDOW_SIZE - 1].SeqNum);
		fprintf(logFile, "Packet[%d]\n\tPacketType: %d\n\tSeqNum: %d\n", (SLIDING_WINDOW_SIZE - 1), packets[SLIDING_WINDOW_SIZE - 1].PacketType, packets[SLIDING_WINDOW_SIZE - 1].SeqNum);

		//break;

		// Send the packets
		for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
		{
			if (packets[i].PacketType != UNINITIALISED)
			{
				ssize_t r = sendto(transmitterSocket, &packets[i], sizeof(struct packet), 0, &netEmuSvr, sizeof(netEmuSvr));
				fprintf(stdout, "Sent Packet[%d] to %s:%d\n", i, inet_ntoa(netEmuSvr.sin_addr), ntohs(netEmuSvr.sin_port));
			}
			if (packets[i].WindowSize < BUFLEN)
				break;
		}
		break;
		
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
					allPacketsAckd = 0;
					break;
				}
			}
			
			if (allPacketsAckd)
			{
				break;
			}
		}
	}

	close(transmitterSocket);
	fclose(configFile);
}
