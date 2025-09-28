TARGET = bin/sensor_view
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
		./$(TARGET) -f ./new_sensors.db -n
		./$(TARGET) -f ./new_sensors.db -a "BNO055_01,BNO055,0x28,1701432000,25.5"

default: $(TARGET)

clean:
	rm -f obj/*.o 
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
		gcc -o $@ $?

obj/%.o: src/%.c 
		gcc -c $< -o $@ -Iinclude
