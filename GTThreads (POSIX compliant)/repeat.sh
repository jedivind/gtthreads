make clean
make
gcc -o dining dining_philosophers.c *.o
./dining
rm ./dining
make clean
