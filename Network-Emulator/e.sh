clear
gcc -c -Wall -o ../l ../Log.c 
gcc -c -Wall -o nem NetworkEmulator.c
gcc ../l nem -o ne
./ne

#{ echo "25"; echo "2"; } | ./ne