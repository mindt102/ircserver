FROM alpine:3.14 as builder
RUN apk add --no-cache build-base
WORKDIR /usr/src/myapp
COPY ./client /usr/src/myapp
COPY ./shared /usr/src/myapp/shared
RUN C_INCLUDE_PATH=$(pwd)/shared gcc client.c shared/json.c shared/shared_func.c -o ./client

FROM alpine:3.14
WORKDIR /usr
ENV ENC_HOST=irc_enc
COPY --from=builder /usr/src/myapp/client /usr/local/bin
