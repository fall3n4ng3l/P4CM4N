FROM alpine:latest

RUN apk add --no-cache gcc musl-dev ncurses-dev make

WORKDIR /app

COPY . /app

RUN make

WORKDIR /app

ENTRYPOINT ["/bin/sh", "-c"]