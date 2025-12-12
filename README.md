# CTFd Helper

This project provides a set of statically compiled binaries and a container image to simplify the deployment and management of CTF (Capture The Flag) challenges using [CTFd](https://ctfd.io/).

## Binaries

- **/bin/init**: Entrypoint binary that reads `/tasks` and executes commands, manages environment variables, and handles signals.
- **/bin/tcpxy**: Simple TCP proxy/listener that executes a command for each incoming connection (useful for exposing challenge services). (a nod to OpenWRT's udpxy)
- **/bin/flag**: Fetches a flag from CTFd and writes it to `flag.txt` for challenge validation.

## Deriving Your Own Challenge Container

This image is designed to be used as a base for your own CTF challenges. Here’s an example `Dockerfile` for a typical challenge:

```
# Build your challenge binaries here
FROM docker.io/alpine:latest AS build
RUN apk add --no-cache gcc musl-dev
RUN mkdir /src
COPY ./challenge.c /src
RUN gcc /src/challenge.c -static -o /src/challenge

FROM ghcr.io/ahmubashshir/ctfd-helper:latest
WORKDIR /
EXPOSE 20240
# Add your configs below
COPY --from=build /src/challenge /challenge
COPY ./tasks /tasks
ENTRYPOINT [ "/bin/init" ]
```


## Documentation

- [docs/tasks.md](./docs/tasks.md): Task file syntax and examples.
- [docs/binaries.md](./docs/binaries.md): Details on helper binaries.
- [docs/example/](docs/example/): Minimal, deployable example challenge tree.

## License

This project is released into the public domain under [The Unlicense](LICENSE).
