CC=gcc
#コンパイル時のフラグ -g gdbでのデバッグ用 -lrt(getclock_) -lm(math.h)
CFLAGS =-g -O3
#リンク時のフラグ
LDFLAGS =-O3 -lrt -lm 

#コンパイルファイル
MAIN += main.o adConvert.o sensor.o lps331.o
GPIO += gpio.o gpio-spi.o gpio-util.o gpio-timer.o gpio-i2c.o gpio-arm-timer.o
OBJ += $(MAIN) $(GPIO)

#ソースコードの検索先
VPATH = ../gpio/

#オブジェクトファイルの出力先
OBJ_DIR = ../obj/

#実行ファイル名
TARGET=sensor


#先に出力先のディレクトリの作成を行う
all: $(OBJ_DIR) $(TARGET)

$(OBJ_DIR):
	mkdir -p $@
	
$(TARGET): $(addprefix $(OBJ_DIR), $(OBJ))
	$(CC) $(LDFLAGS) $(addprefix $(OBJ_DIR), $(OBJ)) -o $@
	
$(OBJ_DIR)%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(addprefix $(OBJ_DIR), $(MAIN))

clean-main:
	rm $(OBJ_DIR)main.o
	
	