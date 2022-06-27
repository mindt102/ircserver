rm -rf test.db
gcc ./migration.c -o migration -l sqlite3 && ./migration
gcc irc.server.c json.c shared_func.c -o irc.server
gcc enc.server.c json.c shared_func.c -o enc.server
gcc auth.server.c json.c shared_func.c -l sqlite3 -lssl -lcrypto -o auth.server
gcc client.c json.c shared_func.c -o client
