CC=gcc
#コンパイル時のフラグ -g gdbでのデバッグ用 -lrt(getclock_) -lm(math.h)
CFLAGS =-g -O3
#リンク時のフラグ
LDFLAGS =-O3 -lrt -lm 

#コンパイルファイル
MAIN += main.o
SRC  += adConvert.o sensor.o lps331.o touchSensor.o
GPIO += gpio.o gpio-spi.o gpio-util.o gpio-timer.o gpio-i2c.o gpio-arm-timer.o
COM_OBJ += $(SRC) $(GPIO)

#ソースコードの検索先
VPATH = ../gpio/

#オブジェクトファイルの出力先
#main_dirはvpath内にあるものと被ると動かないので随時変更->カレントディレクトリ名を使用するように
CUR_DIR = $(shell basename $(CURDIR))
CURRENT_OBJ_DIR = ./$(CUR_DIR)-obj/
COM_OBJ_DIR = ../obj/

#出力先ごとのオブジェクトファイルのパス
MAIN_O_P = $(addprefix $(CURRENT_OBJ_DIR), $(MAIN))
COM_O_P = $(addprefix $(COM_OBJ_DIR), $(COM_OBJ))

#実行ファイル名
TARGET=sensor


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
	