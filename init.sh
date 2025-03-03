#!/bin/bash

if [ "$1" = "b" ] || [ "$1" = "build" ]; then
    echo "Building Docker image..."
    docker build -t pacman .
elif [ "$1" = "s" ] || [ "$1" = "server" ]; then
    if [ -z "$2" ]; then
        echo "Error: players_count parameter is required for server mode."
        echo "Usage: ./init.sh s <players_count>"
        exit 1
    fi
    PLAYERS_COUNT=$2
    echo "Starting server container with PLAYERS_COUNT=$PLAYERS_COUNT..."
    docker run --name pacman-server -p 4444:4444 -it --rm pacman "make server PLAYERS_COUNT=$PLAYERS_COUNT"
elif [ "$1" = "c" ] || [ "$1" = "client" ]; then
    HOST=${2:-127.0.0.1}
    echo "Starting client container with HOST=$HOST..."
    docker run --name pacman-client -it --rm pacman "make client HOST=$HOST"
else
    echo "Usage: ./init.sh [b/build | s/server | c/client [hostname]]"
fi

