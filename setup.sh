rm -rf irc.db
export C_INCLUDE_PATH=$(pwd)/shared
gcc tools/migration.c -o migration -l sqlite3 && ./migration
gcc irc/irc.server.c shared/json.c shared/shared_func.c -o irc.server
gcc enc/enc.server.c shared/json.c shared/shared_func.c -o enc.server
case "$(uname -s)" in

Darwin)
    gcc auth/auth.server.c shared/json.c shared/shared_func.c -lsqlite3 -o auth.server
    ;;

Linux)
    gcc auth/auth.server.c shared/json.c shared/shared_func.c -lsqlite3 -lssl -lcrypto -o auth.server
    ;;

esac

gcc client/client.c shared/json.c shared/shared_func.c -o irc_client