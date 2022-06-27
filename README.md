# Quick Start
0. Compile all servers and client
```
chmod +x setup.sh
./setup.sh
```
1. Start the encryption server
```
./enc.server
```
2. Start the encryption server
```
./auth.server
```
3. Start the IRC server
```
./irc.server
```
4. Start another IRC server and connect to the first one
```
./irc.server 8785 localhost 8784
```
5. Start a client, connect to the first IRC server
```
./client
```
6. Start another client, connect to the second IRC server
```
./client localhost 8785
```
7. Start sending messages