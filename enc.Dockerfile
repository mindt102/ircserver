FROM alpine:3.14 as builder
RUN apk add --no-cache build-base
WORKDIR /usr/src/myapp
COPY ./enc /usr/src/myapp
COPY ./shared /usr/src/myapp/shared
RUN C_INCLUDE_PATH=$(pwd)/shared gcc enc.server.c shared/json.c shared/shared_func.c -o ./enc.server

FROM alpine:3.14
WORKDIR /usr
COPY --from=builder /usr/src/myapp/enc.server /usr/enc.server
CMD ["./enc.server"]