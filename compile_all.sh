gcc irc.server.c json.c shared_func.c -o irc.server
gcc enc.server.c json.c shared_func.c -o enc.server
gcc auth.server.c json.c shared_func.c -o auth.server
gcc client.c json.c shared_func.c -o client