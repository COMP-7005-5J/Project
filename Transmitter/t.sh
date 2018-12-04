clear
gcc -c -Wall -o ../l ../Log.c 
gcc -c -Wall -o tra Transmitter.c
gcc ../l tra -o tr
./tr

#{ echo "localhost"; echo "8003"; echo "1.txt"; } | ./tr