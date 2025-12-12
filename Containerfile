FROM docker.io/alpine:latest AS helper

RUN apk add --no-cache gcc musl-dev
COPY src /src
RUN <<EOF
mkdir /helper
gcc /src/init.c -static -o /helper/init
gcc /src/flag.c -static -o /helper/flag
gcc /src/tcpxy.c -static -o /helper/tcpxy
EOF

FROM scratch
LABEL org.opencontainers.image.description "Helpers for CTFd container based challanges"
LABEL org.opencontainers.image.source https://github.org/ahmubashshir/ctfd-helper

COPY --from=helper /helper /bin
WORKDIR /
# change this if you don't use /bin/tcpxy in tasks
EXPOSE 20240

# expose the port application is reachable on
ENTRYPOINT [ "/bin/init" ]
