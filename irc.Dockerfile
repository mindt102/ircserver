FROM alpine:3.14 as builder
RUN apk add --no-cache build-base
WORKDIR /usr/src/myapp
COPY ./irc /usr/src/myapp
COPY ./shared /usr/src/myapp/shared
RUN C_INCLUDE_PATH=$(pwd)/shared gcc irc.server.c shared/json.c shared/shared_func.c -o ./irc.server

FROM alpine:3.14
WORKDIR /usr
RUN apk add --no-cache bash
COPY --from=builder /usr/src/myapp/irc.server /usr/irc.server
ENV AUTH_HOST=irc_auth
ENV ENC_HOST=irc_enc
CMD ["./irc.server"]
