#ifndef PACKET_H_
#define PACKET_H_

#pragma once

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

#endif