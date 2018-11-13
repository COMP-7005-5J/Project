#include <stdio.h> // FILE
#include <stdlib.h> // fprintf()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // getsockname()
#include <unistd.h> // sleep()
#include <errno.h> // error
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
	int AvgDelay;
	int BitErrorRate;
	struct packet Packet;
};

void forward(struct forwardArgs args)
{
	FILE *logFile = fopen("./logEmulator.txt", "a");
	fprintf(stdout, "forward()\n");
	fprintf(logFile, "forward()\n");
	sleep(args.AvgDelay);
}

int main()
{
	char buffer[BUFLEN];
	char networkIP[16], networkPort[6], receiverIP[16], receiverPort[6];
	FILE *configFile = fopen("./config.txt", "r");
	FILE *logFile = fopen("./logEmulator.txt", "w");
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
	struct sockaddr_in testSvrInfo;
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
	bind(emulatorSocket, (struct sockaddr *) &netEmuSvr, &len);
	fprintf(stdout, "Binded socket\n");
	fprintf(logFile, "Binded socket\n");
	
	// Set up destination server
	recSvr.sin_family = AF_INET;
	inet_aton(receiverIP, &recSvr.sin_addr);
	recSvr.sin_port = htons(atoi(receiverPort));
	fprintf(stdout, "Created destination server\n");
	fprintf(stdout, "\tAddress: %s\n", inet_ntoa(recSvr.sin_addr));
	fprintf(stdout, "\tPort: %d\n", ntohs(recSvr.sin_port));
	fprintf(logFile, "Created destination server\n");
	fprintf(logFile, "\tAddress: %s\n", inet_ntoa(recSvr.sin_addr));
	fprintf(logFile, "\tPort: %d\n", ntohs(recSvr.sin_port));
	
	// Get the BER
	fprintf(stdout, "Enter your desired Bit Error Rate % (int)\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	args.BitErrorRate = atoi(buffer);
	memset(buffer, 0, BUFLEN); // Reset buffer
	fprintf(stdout, "BER: %d\n", args.BitErrorRate);
	fprintf(logFile, "BER: %d\n", args.BitErrorRate);
	
	// Get the Avg Delay
	fprintf(stdout, "Enter your desired delay when a packet is received and sent (seconds)\n");
	fgets(buffer, sizeof(buffer), stdin);
	buffer[strlen(buffer) - 1] = '\0';
	args.AvgDelay = atoi(buffer);
	fprintf(stdout, "Avg Delay: %d\n", args.AvgDelay);
	fprintf(logFile, "Avg Delay: %d\n", args.AvgDelay);
	
	while (eotRecvd == 0)
	{
		fromLen = sizeof fromAddr;
		fprintf(stdout, "Waiting for packet on %s:%d...\n", inet_ntoa(netEmuSvr.sin_addr), ntohs(netEmuSvr.sin_port));
		recvfromResult = recvfrom(emulatorSocket, &recvPacket, sizeof(recvPacket), 0, (struct sockaddr*)&fromAddr, &fromLen);
		fprintf(stdout, "Packet received...\n");
		if (recvfromResult > 0)
		{
			if (recvPacket.PacketType == EOT)
			{
				eotRecvd = 1;
			}
			args.Packet = recvPacket;
			pthread_create(&thread, NULL, forward, args); 
    		pthread_join(thread, NULL);
		}
		else
		{
			fprintf(stdout, "Error[%d]: %s\n", errno, strerror(errno));
			fprintf(logFile, "Error[%d]: %s\n", errno, strerror(errno));
			break;
		}
	
	}
	while (eotSent == 0)
	{
		
	}
	fclose(configFile);
	close(emulatorSocket);
	exit(0);
}