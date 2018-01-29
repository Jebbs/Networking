#! /bin/sh

g++ -oserver server.cpp -std=c++11 -lpthread
g++ -oretriever retriever.cpp -std=c++11

./server 8080 &

# Test 1: Retriever accesses real server
./retriever dsfml.org

# Test 2: Retriever accesses valid file from my server
./retriever 127.0.0.1:8080/index.html

# Test 3: Retriever accesses an unauthorized file from my server
./retriever 127.0.0.1:8080/admin.html

# Test 4: Retriever accesses a forbidden file from my server
./retriever 127.0.0.1:8080/passwords.txt

# Test 5: Retriever requests access to a non-existent file from my server
./retriever 127.0.0.1:8080/nonExistent.html

# Test 6: Retrievers sends a malformed request
./retriever 127.0.0.1:8080 somefile.html

echo -e "\nKilling background server process"
killall -9 server
