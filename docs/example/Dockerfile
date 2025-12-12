FROM docker.io/alpine:latest AS build
RUN apk add --no-cache gcc musl-dev
COPY challenge.c /src/
RUN gcc /src/challenge.c -static -o /src/challenge

FROM ghcr.io/ahmubashshir/ctfd-helper:latest
# expose tcpxy port
EXPOSE 20240
WORKDIR /
COPY --from=build /src/challenge /challenge
COPY flag.txt /flag.txt
COPY tasks /tasks
ENTRYPOINT ["/bin/init"]
