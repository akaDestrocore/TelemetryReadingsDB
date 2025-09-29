TARGET_SRV = bin/telemetry_srv
TARGET_CLI = bin/telemetry_cli

CFLAGS = -std=c90 -Wall

SRC_SRV = $(wildcard src/srv/*.c)
OBJ_SRV = $(SRC_SRV:src/srv/%.c=obj/srv/%.o)

SRC_CLI = $(wildcard src/cli/*.c)
OBJ_CLI = $(SRC_CLI:src/cli/%.c=obj/cli/%.o)

run: clean default
	./$(TARGET_SRV) -f ./telemetry_db -n -p 8080

default: 
# Build server
$(TARGET_SRV): $(OBJ_SRV)
	gcc $(CFLAGS) -o $@ $(OBJ_SRV)

$(OBJ_SRV): obj/srv/%.o: src/srv/%.c
	gcc $(CFLAGS) -c $< -o $@ -Iinclude

# Build client
$(TARGET_CLI): $(OBJ_CLI)
	gcc $(CFLAGS) -o $@ $(OBJ_CLI)

$(OBJ_CLI): obj/cli/%.o: src/cli/%.c
	gcc $(CFLAGS) -c $< -o $@ -Iinclude

# Directory creation
directories:
	mkdir -p bin obj/srv obj/cli

# Cleanup targets
cleanup:
	killall -9 dbserver 2>/dev/null || true

clean: cleanup
	rm -f obj/srv/*.o obj/cli/*.o
	rm -f bin/*
	rm -f *.db
