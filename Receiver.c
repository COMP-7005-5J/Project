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

struct packet
{
	int PacketType;
	int SeqNum;
	char data[BUFLEN];
	int WindowSize;
	int AckNum;
};

/*
struct forwardArgs
{
	int *RecvSocket;
	struct packet Packet;
	struct sockaddr_in *DestSvr;
};

void forward(struct forwardArgs *args)
{
	FILE *logFile = fopen("./logEmulator.txt", "a");
	struct packet packet = args->Packet;
	sleep(*(args->AvgDelay));
	if (sendto(*(args->EmulatorSocket), &packet, sizeof(struct packet), 0, args->DestSvr, sizeof(*(args->DestSvr))) < 0)
	{
		fprintf(stdout, "ERROR: Couldn't send packet. %s\n", strerror(errno));
		fprintf(logFile, "ERROR: Couldn't send packet. %s\n", strerror(errno));
	}
	else
	{
		fprintf(stdout, "Forwarded Packet[%d]\n", packet.SeqNum);
		fprintf(logFile, "Forwarded Packet[%d]\n", packet.SeqNum);
	}
}
*/

int main()
{
	char buffer[BUFLEN];
	char networkIP[16], networkPort[6], receiverIP[16], receiverPort[6];
	FILE *configFile = fopen("./config.txt", "r");
	FILE *destFile = fopen("./2.txt", "a");
	FILE *logFile = fopen("./logReceiver.txt", "w");
	int recvSocket;
	int eotRecvd = 0;
	int eotSent = 0;
	int numOfPktsRecvd = 0;
	struct packet *pktsToAck = malloc(1 * sizeof(*pktsToAck));
	//pthread_t threads[SLIDING_WINDOW_SIZE];
	pthread_t thread;
	socklen_t fromLen;
	socklen_t len;
	//struct forwardArgs args;
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
	//args.DestSvr = &netEmuSvr;
	fprintf(stdout, "Created destination server\n");
	fprintf(stdout, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	fprintf(stdout, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	fprintf(logFile, "Created destination server\n");
	fprintf(logFile, "\tAddress: %s\n", inet_ntoa(netEmuSvr.sin_addr));
	fprintf(logFile, "\tPort: %d\n", ntohs(netEmuSvr.sin_port));
	
	int x = 0;
	
	while (1)
	{
		while (eotRecvd == 0)
		{
			if (recvfrom(recvSocket, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr*)&fromAddr, &fromLen) > 0)
			{
				++numOfPktsRecvd;
				pktsToAck = realloc(pktsToAck, numOfPktsRecvd * sizeof(*pktsToAck));
				if (recvPacket.PacketType == EOT)
				{
					eotRecvd = 1;
					pktsToAck[numOfPktsRecvd-1].PacketType = EOT;
				}
				else
				{
					pktsToAck[numOfPktsRecvd-1].PacketType = ACK;
				}
				fprintf(stdout, "Received Packet[%d]\n", recvPacket.SeqNum);
				fprintf(logFile, "Received Packet[%d]\n", recvPacket.SeqNum);
				
				pktsToAck[numOfPktsRecvd-1].SeqNum = recvPacket.AckNum;
				pktsToAck[numOfPktsRecvd-1].AckNum = recvPacket.SeqNum;
				fprintf(destFile, "%s", recvPacket.data);
				
				/*
				args.Packet = recvPacket;
				pthread_create(&thread, NULL, forward, &args); 
    			pthread_join(thread, NULL);
    			*/
			}
			else
			{
				fprintf(stdout, "Error: Couldn't receive packet. %s\n", strerror(errno));
				fprintf(logFile, "Error: Couldn't receive packet. %s\n", strerror(errno));
			}
		}
		
		//args.DestSvr = &fromAddr;
		
		//while (eotSent == 0)
		//{
			for (int i = 0; i < numOfPktsRecvd; i++)
			{
				if (sendto(recvSocket, &pktsToAck[i], sizeof(struct packet), 0, &netEmuSvr, sizeof(netEmuSvr)) < 0)
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
		//}
		numOfPktsRecvd = 0;
		eotRecvd = 0;
		eotSent = 0;
	}
	free(pktsToAck);
	fclose(configFile);
	close(recvSocket);
	return 0;
}