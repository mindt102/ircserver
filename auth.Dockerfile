FROM alpine:3.14 as builder
RUN apk add --no-cache build-base openssl-dev sqlite-libs sqlite-dev bash
WORKDIR /usr/src/myapp
COPY ./auth /usr/src/myapp
COPY ./shared /usr/src/myapp/shared
RUN C_INCLUDE_PATH=$(pwd)/shared gcc auth.server.c shared/json.c shared/shared_func.c -lsqlite3 -lssl -lcrypto -o ./auth.server
COPY ./tools .
RUN gcc -o ./migration migration.c -lsqlite3
RUN ./migration

FROM alpine:3.14
WORKDIR /usr
RUN apk add --no-cache openssl-dev sqlite-dev
COPY --from=builder /usr/src/myapp/auth.server /usr/auth.server
COPY --from=builder /usr/src/myapp/irc.db /usr/irc.db
CMD ["./auth.server"]
