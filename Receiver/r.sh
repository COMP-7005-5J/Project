clear
gcc -c -Wall -o ../l ../Log.c 
gcc -c -Wall -o rec Receiver.c 
gcc ../l rec -o re
./re