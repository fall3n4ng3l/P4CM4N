PORT = 4444
HOST = 127.0.0.1
PLAYERS_COUNT ?= 2
FRAME_RATE = 50

all: pacman

pacman: ./src/pacman.c
	@echo "Compiling..."
	@gcc ./src/pacman.c ./src/net.c -I./src -lncurses -pthread -o ./bin/pacman

server: pacman
	@echo "Running in server mode..."
	@./bin/pacman -s $(PLAYERS_COUNT) -p $(PORT) -r $(FRAME_RATE)

client: pacman
	@echo "Running in client mode..."
	@./bin/pacman -c -h $(HOST) -p $(PORT)

clean:
	@echo "Cleaning..."
	@rm -f ./bin/pacman
