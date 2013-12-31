CC=gcc
CFLAG =-g 

#-lrt(getclock_)
LDFLAG =-O3 -lrt

OBJ_DIR = obj/

SRC = main.o gpio.o gpio-spi.o gpio-util.o gpio-timer.o gpio-i2c.o gpio-arm-timer.o
OBJ = $(addprefix $(OBJ_DIR), $(SRC:.c=.o) )

TARGET=gpio-test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAG) $(OBJ) -o $@
	#sudo ./$(TARGET) -D
	

$(OBJ_DIR)%.o : %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OJB_DIR)
	
clean:
	rm ./obj/*.o