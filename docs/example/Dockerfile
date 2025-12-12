# Build Section
#  ____        _ _     _   ____            _   _
# | __ ) _   _(_) | __| | / ___|  ___  ___| |_(_) ___  _ __
# |  _ \| | | | | |/ _` | \___ \ / _ \/ __| __| |/ _ \| '_ \
# | |_) | |_| | | | (_| |  ___) |  __/ (__| |_| | (_) | | | |
# |____/ \__,_|_|_|\__,_| |____/ \___|\___|\__|_|\___/|_| |_|
#
# build your things here
FROM docker.io/alpine:latest AS build
RUN apk add --no-cache gcc musl-dev
RUN mkdir /src
COPY ./challenge.c /src
RUN gcc /src/challenge.c -static -o /src/challenge


# Container
#   ____            _        _
#  / ___|___  _ __ | |_ __ _(_)_ __   ___ _ __
# | |   / _ \| '_ \| __/ _` | | '_ \ / _ \ '__|
# | |__| (_) | | | | || (_| | | | | |  __/ |
#  \____\___/|_| |_|\__\__,_|_|_| |_|\___|_|
#
# configure container
FROM ghcr.io/ahmubashshir/ctfd-helper:latest
WORKDIR /
EXPOSE 20240
# Add your configs below
COPY --from=build /src/challenge /challenge
COPY flag.txt /flag.txt
COPY tasks /tasks

# expose the port application is reachable on
ENTRYPOINT [ "/bin/init" ]
