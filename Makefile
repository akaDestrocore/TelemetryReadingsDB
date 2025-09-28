TARGET = bin/sensor_view
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
		./$(TARGET) -f ./new_sensors.db -n

default: $(TARGET)

clean:
	rm -f obj/*.o 
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
		gcc -o $@ $? -g

obj/%.o: src/%.c 
		gcc -c $< -o $@ -Iinclude -g
