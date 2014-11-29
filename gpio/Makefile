CC=gcc
CFLAGS =-g -O3
LDFLAGS =-O3 -lrt -lm -lpthread

#コンパイルファイル
MAIN += main.o
GPIO += gpio.o gpio-spi.o gpio-util.o gpio-timer.o gpio-i2c.o gpio-arm-timer.o
COM_OBJ += $(GPIO)

CUR_DIR = $(shell basename $(CURDIR))
CURRENT_OBJ_DIR = ./$(CUR_DIR)-obj/
COM_OBJ_DIR = ../obj/

#出力先ごとのオブジェクトファイルのパス
MAIN_O_P = $(addprefix $(CURRENT_OBJ_DIR), $(MAIN))
COM_O_P = $(addprefix $(COM_OBJ_DIR), $(COM_OBJ))

TARGET=gpio-test

#先に出力先のディレクトリの作成を行う
all: $(COM_OBJ_DIR) $(CURRENT_OBJ_DIR) $(TARGET)

$(CURRENT_OBJ_DIR):
	mkdir -p $@

$(COM_OBJ_DIR):
	mkdir -p $@

	
$(TARGET): $(COM_O_P) $(MAIN_O_P)
	$(CC) $(LDFLAGS) $(COM_O_P) $(MAIN_O_P) -o $@
	
$(CURRENT_OBJ_DIR)%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	
$(COM_OBJ_DIR)%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(addprefix $(COM_OBJ_DIR), $(SRC))
	rm $(MAIN)
	