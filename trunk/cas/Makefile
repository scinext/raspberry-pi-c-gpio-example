CC=gcc

#コンパイラフラグ
CFLAGS =-g -O3

#-lrt(getclock_, POSIXメッセージキュー) -lm(math.h)
LDFLAGS =-O3 -lrt -lm

#オブジェクト(自動でVPATHから検索される)
MAIN += main.o
SRC += mode.o threads.o
SENSOR += adConvert.o sensor.o lps331.o
GPIO += gpio.o gpio-spi.o gpio-util.o gpio-timer.o gpio-i2c.o gpio-arm-timer.o
COM_OBJ += $(SRC) $(SENSOR) $(GPIO)


#ソースコードの依存先を探すディレクトリ
VPATH = ../sensor/ ../gpio/

#オブジェクトファイルの出力先
#main_dirはvpath内にあるものと被ると動かないので随時変更->カレントディレクトリ名を使用するように
CUR_DIR = $(shell basename $(CURDIR))
CURRENT_OBJ_DIR = ./$(CUR_DIR)-obj/
COM_OBJ_DIR = ../obj/

#出力先ごとのオブジェクトファイルのパス
MAIN_O_P = $(addprefix $(CURRENT_OBJ_DIR), $(MAIN))
COM_O_P = $(addprefix $(COM_OBJ_DIR), $(COM_OBJ))

#実行ファイル名
TARGET=../cas.exe


#先に出力先のディレクトリの作成を行う
all: $(COM_OBJ_DIR) $(CURRENT_OBJ_DIR) $(TARGET)

$(COM_OBJ_DIR):
	mkdir -p $@

$(CURRENT_OBJ_DIR):
	mkdir -p $@

$(TARGET): $(MAIN_O_P) $(COM_O_P)
	$(CC) $(LDFLAGS) $(COM_O_P) $(MAIN_O_P) -o $@

$(CURRENT_OBJ_DIR)%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(COM_OBJ_DIR)%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	#rm -rf ./obj/
	#rm $(addprefix $(COM_OBJ_DIR), $(MAIN))
	rm $(addprefix $(COM_OBJ_DIR), $(SRC))
	rm $(MAIN)

#clean-main:
#	rm $(COM_OBJ_DIR)main.o


#
#
#MAIN_OBJ = main.o mode.o threads.o sensor.o
#SENSR_OBJ = adConvert.o
#GPIO_OBJ = gpio.o gpio-spi.o gpio-util.o gpio-timer.o gpio-i2c.o gpio-arm-timer.o
#
#$(TARGET): $(MAIN_OBJ) $(SENSR_OBJ) $(GPIO_OBJ)
#
#$(MAIN_OBJ): %.o: %.c
#	#$(CC) $(CFLAGS) -c $< -o $(OBJ_DIR)$@
#
#$(SENSR_OBJ): %.o: %.c
#	#$(CC) $(CFLAGS) -c $< -o $(SENSOR_OBJ_DIR)$@
#
#$(GPIO_OBJ): %.o: %.c
#	#$(CC) $(CFLAGS) -c $< -o $(GPIO_OBJ_DIR)$@
#
#
