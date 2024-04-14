预装openssl
编译:
gcc sever.c -o bin/sever.out -lssl -pthread -crypto
gcc client.c -o bin/client.out -lssl -pthread -crypto
