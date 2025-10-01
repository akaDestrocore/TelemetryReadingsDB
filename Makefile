CFLAGS = -Wall -Iinclude

TARGET_SRV = bin/telemetry_srv
TARGET_CLI = bin/telemetry_cli

SRC_SRV = $(wildcard src/srv/*.c)
OBJ_SRV = $(patsubst src/srv/%.c,obj/srv/%.o,$(SRC_SRV))

SRC_CLI = $(wildcard src/cli/*.c)
OBJ_CLI = $(patsubst src/cli/%.c,obj/cli/%.o,$(SRC_CLI))

.PHONY: default run clean directories

run: default
	./$(TARGET_SRV) -f ./telemetry_db.db -n -p 8080

default: directories $(TARGET_SRV) $(TARGET_CLI)

# Link targets
$(TARGET_SRV): $(OBJ_SRV)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_CLI): $(OBJ_CLI)
	$(CC) $(CFLAGS) -o $@ $^

obj/srv/%.o: src/srv/%.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

obj/cli/%.o: src/cli/%.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure directories exist
directories:
	mkdir -p bin obj/srv obj/cli

# Cleanup
clean:
	killall -9 dbserver 2>/dev/null || true
	rm -f obj/srv/*.o obj/cli/*.o
	rm -f bin/*
	rm -f *.db
