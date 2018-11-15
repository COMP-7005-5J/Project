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

FILE *logFile;
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
	char networkIP[16], networkPort[6], receiverIP[16], receiverPort[6];
	FILE *configFile = fopen("./config.txt", "r");
	FILE *destFile;
	logFile = fopen("./logReceiver.txt", "w+");
	int recvSocket;
	int duplicatePktRecvd = 0;
	int eotRecvd = 0;
	int numOfPktsRecvd = 0;
	struct packet *pktsToAck = malloc(1 * sizeof(*pktsToAck));
	socklen_t fromLen;
	socklen_t len;
	struct packet recvPacket;
	struct sockaddr_in recSvr;
	struct sockaddr_in netEmuSvr;
	struct sockaddr_in fromAddr;
	
	fromLen = sizeof(fromAddr);
	
	// Get the configurations
	fscanf(configFile, "%s %s %s %s", networkIP, networkPort, receiverIP, receiverPort);
	fprintf(stdout, "Loaded configurations\n");
	fprintf(logFile, "Loaded configurations\n");

	// Create socket
	recvSocket = socket(AF_INET, SOCK_DGRAM, 0);
	fprintf(stdout, "Created socket\n");
	fprintf(logFile, "Created socket\n");
	
	// Set up receiver server
	bzero((char*)&recSvr,sizeof(struct sockaddr_in));
	recSvr.sin_family = AF_INET;
	recSvr.sin_addr.s_addr = inet_addr(receiverIP);
	recSvr.sin_port = htons(atoi(receiverPort));
	fprintf(stdout, "Created receiver's server\n");
	fprintf(stdout, "\tAddress: %s\n", inet_ntoa(recSvr.sin_addr));
	fprintf(stdout, "\tPort: %d\n", ntohs(recSvr.sin_port));
	fprintf(logFile, "Created receiver's server\n");
	fprintf(logFile, "\tAddress: %s\n", inet_ntoa(recSvr.sin_addr));
	fprintf(logFile, "\tPort: %d\n", ntohs(recSvr.sin_port));
	
	// Bind socket
	len = sizeof(recSvr);
	if (bind(recvSocket, (struct sockaddr *) &recSvr, len) < 0)
	{
		fprintf(stdout, "ERROR: Couldn't bind socket. %s\n", strerror(errno));
		fprintf(logFile, "ERROR: Couldn't bind socket. %s\n", strerror(errno));
		exit(0);
	}
	//args.RecvSocket = &recvSocket;
	fprintf(stdout, "Binded socket\n");
	fprintf(logFile, "Binded socket\n");
	
	// Set up the network emulator server
	netEmuSvr.sin_family = AF_INET;
	inet_aton(networkIP, &netEmuSvr.sin_addr);
	netEmuSvr.sin_port = htons(atoi(networkPort));
	fprintf(stdout, "Created destination server\n");
	fprintf(stdout, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	fprintf(stdout, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	fprintf(logFile, "Created destination server\n");
	fprintf(logFile, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	fprintf(logFile, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	
	fprintf(stdout, "STARTING SERVICE\n\n");
	fprintf(logFile, "STARTING SERVICE\n\n");
	destFile = fopen("./2.txt", "w+");
	fclose(destFile);
	fclose(logFile);
	while (1)
	{
		logFile = fopen("./logReceiver.txt", "a");
		destFile = fopen("./2.txt", "r+");
		while (eotRecvd == 0)
		{
			// Receive packets
			if (recvfrom(recvSocket, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr*)&fromAddr, &fromLen) > 0)
			{
				// Check if the packet received is a duplicate
				for (int i = 0; i < numOfPktsRecvd; i++)
				{
					if (recvPacket.SeqNum == pktsToAck[i].AckNum)
					{
						duplicatePktRecvd = 1;
						break;
					}
				}
				
				// If the packet received isn't a duplicate, we add it to the list of packets to acknowledge
				if (duplicatePktRecvd == 0)
				{
					// Extent array to store another packet
					++numOfPktsRecvd;
					pktsToAck = realloc(pktsToAck, numOfPktsRecvd * sizeof(*pktsToAck));
					fprintf(stdout, "Received ");
					fprintf(logFile, "Received ");
					if (recvPacket.PacketType == EOT)
					{
						eotRecvd = 1;
						pktsToAck[numOfPktsRecvd-1].PacketType = EOT;
						fprintf(stdout, "EOT");
						fprintf(logFile, "EOT");
					}
					else
					{
						pktsToAck[numOfPktsRecvd-1].PacketType = ACK;
						fprintf(stdout, "DATA");
						fprintf(logFile, "DATA");
					}
					fprintf(stdout, "[%d]  \tLEN: %d\tACK: %d\n", recvPacket.SeqNum, recvPacket.WindowSize, recvPacket.AckNum);
					fprintf(logFile, "[%d]  \tLEN: %d\tACK: %d\n", recvPacket.SeqNum, recvPacket.WindowSize, recvPacket.AckNum);
				
					// Put packet into array to keep track of what to ACK
					pktsToAck[numOfPktsRecvd-1].SeqNum = recvPacket.AckNum;
					pktsToAck[numOfPktsRecvd-1].AckNum = recvPacket.SeqNum;
					
					// Extend the file if the received data needs to be appended
					fseek(destFile, (recvPacket.AckNum-recvPacket.WindowSize-1), SEEK_SET);					
					fwrite(recvPacket.data, sizeof(char), recvPacket.WindowSize, destFile);
				}
				else
				{
					duplicatePktRecvd = 0;
				}
			}
			else
			{
				fprintf(stdout, "Error: Couldn't receive packet. %s\n", strerror(errno));
				fprintf(logFile, "Error: Couldn't receive packet. %s\n", strerror(errno));
			}
		}
		
		fprintf(stdout, "\n");
		fprintf(logFile, "\n");
		
		// Send ACKs for each packet received
		for (int i = 0; i < numOfPktsRecvd; i++)
		{
			if (sendto(recvSocket, &pktsToAck[i], sizeof(struct packet), 0, (struct sockaddr*)&netEmuSvr, sizeof(netEmuSvr)) < 0)
			{
				fprintf(stdout, "Error: Couldn't send packet. %s\n", strerror(errno));
				fprintf(logFile, "Error: Couldn't send packet. %s\n", strerror(errno));
			}
			else
			{
				fprintf(stdout, "Sent ACK[%d]\n", pktsToAck[i].AckNum);
				fprintf(logFile, "Sent ACK[%d]\n", pktsToAck[i].AckNum);
			}
		}
		
		fprintf(stdout, "\n");
		fprintf(logFile, "\n");
		
		// Reset variables
		numOfPktsRecvd = 0;
		eotRecvd = 0;
		free(pktsToAck);
		pktsToAck = malloc(1 * sizeof(*pktsToAck));
		fclose(destFile);
		fclose(logFile);
	}
	free(pktsToAck);
	fclose(configFile);
	close(recvSocket);
	return 0;
}