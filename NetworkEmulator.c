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
struct forwardArgs
{
	int *AvgDelay;
	int *BitErrorRate;
	int *EmulatorSocket;
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

int main()
{
	char buffer[BUFLEN];
	char networkIP[16], networkPort[6], receiverIP[16], receiverPort[6];
	FILE *configFile = fopen("./config.txt", "r");
	FILE *logFile = fopen("./logEmulator.txt", "w");
	int avgDelay;
	int bitErrorRate;
	int emulatorSocket;
	int eotRecvd = 0;
	int eotSent = 0;
	int recvfromResult = 0;
	//pthread_t threads[SLIDING_WINDOW_SIZE];
	pthread_t thread;
	socklen_t fromLen;
	socklen_t len;
	struct forwardArgs args;
	struct packet recvPacket;
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
	args.EmulatorSocket = &emulatorSocket;
	fprintf(stdout, "Binded socket\n");
	fprintf(logFile, "Binded socket\n");
	
	// Set up destination server
	recSvr.sin_family = AF_INET;
	inet_aton(receiverIP, &recSvr.sin_addr);
	recSvr.sin_port = htons(atoi(receiverPort));
	args.DestSvr = &recSvr;
	fprintf(stdout, "Created destination server\n");
	fprintf(stdout, "\tAddress: %s\n", inet_ntoa(args.DestSvr->sin_addr));
	fprintf(stdout, "\tPort: %d\n", ntohs(args.DestSvr->sin_port));
	fprintf(logFile, "Created destination server\n");
	fprintf(logFile, "\tAddress: %s\n", inet_ntoa(args.DestSvr->sin_addr));
	fprintf(logFile, "\tPort: %d\n", ntohs(args.DestSvr->sin_port));
	
	// Get the BER
	fprintf(stdout, "Enter your desired Bit Error Rate percentage (int)\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	bitErrorRate = atoi(buffer);
	args.BitErrorRate = &bitErrorRate;
	memset(buffer, 0, sizeof(buffer)); // Reset buffer
	fprintf(stdout, "BER: %d\n", *(args.BitErrorRate));
	fprintf(logFile, "BER: %d\n", *(args.BitErrorRate));
	
	// Get the Avg Delay
	fprintf(stdout, "Enter your desired delay when a packet is received and sent (seconds)\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	avgDelay = atoi(buffer);
	args.AvgDelay = &avgDelay;
	fprintf(stdout, "Avg Delay: %d\n", *(args.AvgDelay));
	fprintf(logFile, "Avg Delay: %d\n", *(args.AvgDelay));
	
	while (1)
	{
		while (eotRecvd == 0)
		{
			fromLen = sizeof fromAddr;
			recvfromResult = recvfrom(emulatorSocket, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr*)&fromAddr, &fromLen);
			if (recvfromResult > 0)
			{
				if (recvPacket.PacketType == EOT)
				{
					eotRecvd = 1;
				}
				args.Packet = recvPacket;
				pthread_create(&thread, NULL, forward, &args); 
    			pthread_join(thread, NULL);
			}
			else
			{
				fprintf(stdout, "Error: Couldn't receive packets. %s\n", strerror(errno));
				fprintf(logFile, "Error: Couldn't receive packets. %s\n", strerror(errno));
				break;
			}
		}
		
		args.DestSvr = &fromAddr;
		
		while (eotSent == 0)
		{
			recvfromResult = recvfrom(emulatorSocket, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr*)&fromAddr, &fromLen);
			if ((fromAddr.sin_addr.s_addr == recSvr.sin_addr.s_addr) && (fromAddr.sin_port == recSvr.sin_port))
			{
				if (recvPacket.PacketType == EOT)
				{
					eotSent = 1;
				}
				args.Packet = recvPacket;
				pthread_create(&thread, NULL, forward, &args); 
    			pthread_join(thread, NULL);
			}
			else
			{
				fprintf(stdout, "Error: Couldn't receive packets. %s\n", strerror(errno));
				fprintf(logFile, "Error: Couldn't receive packets. %s\n", strerror(errno));
				break;
			}
		}
	}
	fclose(configFile);
	close(emulatorSocket);
	exit(0);
}