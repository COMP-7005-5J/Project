#define BUFLEN 255
#define SLIDING_WINDOW_SIZE 4
#define DATA 0
#define EOT 1
#define ACK 2
struct packet
{
	int PacketType;
	int SeqNum;
	char data[PayloadLen];
	int WindowSize;
	int AckNum;
}
int main()
{
	char buffer[BUFLEN];
	char networkIP[16], networkPort[5];
	File *configFile = fopen(“config.txt”, “r”);
	File *logFile = fopen("log.txt", "a");
	File *fileToSend;
	int seqNum = 1;
	int transmitterSocket;
	struct packet packets[SLIDING_WINDOW_SIZE];
	ssize_t numOfElementsRead = 0;
	struct sockaddr_in netEmuSvr;

	// Get the network emulator’s configurations
	fscanf(configFile, "%s %s %*s %*s", networkIP, networkPort);
	fprintf(stdout, "Got network IP and port number\n");
	fprintf(logFile, "Got network IP and port number\n");

	// Create socket
	transmitterSocket = socket(AF_INET, SOCK_DGRAM, protocol);
	fprintf(stdout, "Created socket\n");
	fprintf(logFile, "Created socket\n");

	// Set up destination server
	netEmuSvr.sin_family = AF_INET;
	netEmuSvr.sin_addr.s_addr = inet_addr(networkIP);
	netEmuSvr.sin_port = htons(networkPort);

	// Open the file to send
	fprintf(stdout, "Enter name of file\n");
	fgets(buffer, sizeof(buffer), stdin);
	fileToSend = fopen(buffer, “r”);
	fprintf(stdout, "Opened file to send\n");
	fprintf(logFile, "Opened file to send\n");
	memset(buffer, 0, BUFLEN); // Reset buffer

	// Loop forever until there’s nothing else to do
	while (1)
	{
		// Create the packets
		for (int i = 0; i < SLIDING_WINDOW_SIZE; i++, seqNum++)
		{
		packets[i] = (packet)
		{
			.PacketType = DATA,
			.SeqNum = seqNum
		};
		packets[i].WindowSize = fread(packets[i].data, sizeof(buffer), sizeof(char), fileToSend);
		packets[i].AckNum = packets[i].SeqNum * packets[i].WindowSize + 1;
	
		// If the window size is less than BUFFER length, then we have the last bits of data
		if (packets[i].WindowSize < BUFLEN)
			break;
		}

		// Send the packets
		for (int i = 0; i < SLIDING_WINDOW_SIZE; i++)
		{
			if (packets[i] != NULL)
				sendto(transmitterSocket, &packets[i], sizeof(struct packet), 0, &netEmuSvr, sizeof(netEmuSvr));
			if (packets[i].WindowSize < BUFLEN)
				break;
		}
	}

	fclose(configFile);
}
