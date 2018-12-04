# Project
 A basic Send-And-Wait protocol simulator. The protocol is half-duplex and it uses sliding windows to send multiple packets between 2 hosts on a LAN with an “unreliable network” between the 2 hosts.

# Packet Loss
 In the Network Emulator, when the forward() method is called, a random number from 0-99 is picked. If the random number is larger than the BER entered by the user at the start of the program, then a log is generated and nothing else is done with the packet. The forward() method is called when packets are received from both the Transmitter and the Receiver.
 
# Duplicate Packets
 If the Receiver receives a packet that it has already sent an ACK for, then nothing is done with the received packet and the Receiver waits for the EOT or more DATA to arrive.
 
# Timeouts
 A timeout only occurs when the transmitter doesn't receive and EOT from the Receiver in a set amount of time (Network Emulator dropped the Receiver's EOT packet). When that happens, the window will slide until the first slot is taken by a packet that hasn't been ACK'd. Then it will resend all the packets it didn't receive an ACK for, along with any packets that are now within the window's frame after it slid.
 
 Ex.
 Window when packets were initially sent: { DATA[1], DATA[2], DATA[3], EOT[4] }
 Window when packets when ACKs arrived: { -1-, -2-, DATA[3], EOT[4] }
 Window when packets are to be sent again: { DATA[3], DATA[4], DATA[5], EOT[6] }
 
# Network Emulator
 The Network Emulator sends/receives packets from the Transmitter and the Receiver. For the Receiver, the Network Emulator knows its server configurations (IP address and port number) by taking it from the configuration file. As for the Transmitter's server configurations, the Network Emulator gets it and stores it when it receives packets via (recvfrom). It's able to do this because one of the arguments is a structure that gets instantiated with all the info when it receives a packet. The first packet to arrive of type EOT determines which Transmitter will receive ACKs.
 
# Configuration File
 The configuration file contains the IP address and port numbers for the Network Emulator and Receiver. As for the Transmitter, it prompts the user to specify the IP address and port number (if the user doesn't specify the port, 8080 is used by default). This is done because to make the project more realistic: the Network Emulator can never know who will send packets its way and when. But, it must know the Receiver's server configurations so that it knows where to forward the packets to.
 
# Half-Duplex
 The Network Emulator can only be in 1 of 4 states:
 - Receiving packets from a Transmitter
 - Sending packets to the Receiver
 - Receiving ACKs from the Receiver
 - Sending ACKs to the Transmitter
 
 This means the Network Emulator can only receive or send (not both) packets in one direction at a time.
 
# Sliding Window
 The Transmitter has a sliding window of a set length (as of Nov. 15th: 4). The Transmitter determines which packets in the window to send based on if its packet type is UNINITIALISED. When ACKs are received, the packets that have been ACK'd have their type set to UNINITIALISED. The window scrolls by finding the distance between its first slot and the slot containing a packet that isn't UNINITIALISED. Once it figures that out, it sets arguments of packet #[former position - scroll distance] to match the arguments of packet #[former position] and then setting packet type argument of the packet#[former position] to UNINITIALISED. When the opportunity to make new packets arrives, the Transmitter decides how many packets to create based on the distance that the window has shift. So if the window didn't shift, no new packets would be created.
 
 Ex.
 1. Window when packets were initially sent: { DATA[1], DATA[2], DATA[3], EOT[4] }
 2. Window when packets when ACKs arrived: { -1-, -2-, DATA[3], -4- }
 3. Window after shifting the packets: { DATA[3], -2-, DATA[5], DATA[6] } : Scrolling distance is set to 2, therefore 2 packets are created at the end of the window.
 4. Window when packets are to be sent again: { DATA[3], -2-, DATA[5], EOT[6] }

 Another Ex.
 1. Window when packets were initially sent: { DATA[1], DATA[2], DATA[3], EOT[4] }
 2. Window when packets when ACKs arrived: { DATA[1], -2-, -3-, -4- }
 3. Window after shifting the packets: { DATA[1], -2-, -3-, -4- } : Scrolling distance is set to 0, therefore no packets are created at the end of the window.
 4. Window when packets are to be sent again: { DATA[3], -2-, -3-, -4- }
 
# How To Run
 Run each of these on a different terminal tab/window OR computer:
 1. Run the Emulator shell (Network-Emulator/e.sh)
 2. Run the Receiver shell (Receiver/r.sh)
 3. Run the Transmitter shell (Transmitter/t.sh)
 
Shells were used because in order to compile the code while also reducing redundancy, 2 classes need to be compiled and executed. This requires multiple lines entered into the command prompt/terminal, so to save time, bash shells were implemented.