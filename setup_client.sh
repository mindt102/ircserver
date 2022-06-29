#!/usr/bin/bash
export C_INCLUDE_PATH=$(pwd)/shared
gcc client/client.c shared/json.c shared/shared_func.c -o irc_client
