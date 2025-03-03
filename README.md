# Simple Multiplayer Pac-Man Game (Written in C)

A 2–4 player game where the goal is to collect the most points by eating food on a procedurally generated, symmetrical map. Players move in a chosen direction until hitting an obstacle (walls or other players) and can change direction with keypresses. Every food cell on the map must be reachable by all players.

## Game End Condition
- The game ends when no food cells remain on the map.
- Players earn 1 point per collected food cell.
- If scores are tied, the game ends in a draw.

## Execution
- **Server:** Generates the map, opens a listening port, and waits for players.
- **Client:** Connects to the server’s address and port.
- Once all players signal readiness, the server starts the game.
- During the game, only keypresses are transmitted over the network.

## Build and Run
Game parameters can be adjusted in the `Makefile`. Use the following commands:
- `make server` — builds and starts the server.
- `make client` — builds and starts the client.

## Docker Setup
An executable script builds a Docker image and runs both the server and client in containers.  
Make sure to specify your local IP address as `host` (e.g., `192.168.1.4`).
