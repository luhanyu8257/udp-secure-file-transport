预装openssl
编译:
gcc server.c -o bin/server.out -lssl -pthread -crypto
gcc client.c -o bin/client.out -lssl -pthread -crypto
