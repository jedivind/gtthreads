make clean
make
gcc -o test main.c gtthread.a
./test
rm ./test
make clean
