FROM scratch AS metas
LABEL org.opencontainers.image.description="Helpers for CTFd container based challanges"
LABEL org.opencontainers.image.source="https://github.org/ahmubashshir/ctfd-helper"

FROM docker.io/alpine:latest AS helper

RUN apk add --no-cache gcc musl-dev
COPY src /src
RUN mkdir /helper
RUN gcc /src/init.c -static -o /helper/init
RUN gcc /src/flag.c -static -o /helper/flag
RUN gcc /src/tcpxy.c -static -o /helper/tcpxy

FROM metas

COPY --from=helper /helper /bin
WORKDIR /
# change this if you don't use /bin/tcpxy in tasks
EXPOSE 20240

# expose the port application is reachable on
ENTRYPOINT [ "/bin/init" ]
