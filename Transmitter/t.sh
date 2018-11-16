clear
gcc -c -Wall -o ../l ../Log.c 
gcc -c -Wall -o tra Transmitter.c
gcc ../l tra -o tr
echo "1.txt" | ./tr