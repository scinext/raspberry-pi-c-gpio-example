CC=gcc

#コンパイラフラグ
CFLAGS =-g -O3

#-lrt(getclock_, POSIXメッセージキュー) -lm(math.h)
LDFLAGS =-O3 -lrt -lm 


#ソースコードの依存先を探すディレクトリ
VPATH = ../sensor/ ../gpio/

#オブジェクト(自動でVPATHから検索される)
MAIN += main.o mode.o threads.o sensor.o
MAIN += adConvert.o
GPIO += gpio.o gpio-spi.o gpio-util.o gpio-timer.o gpio-i2c.o gpio-arm-timer.o
OBJ += $(MAIN) $(GPIO)


#オブジェクトファイルの出力先
OBJ_DIR = ../obj/

TARGET=../cas.exe

#先に出力先のディレクトリの作成を行う
all: $(OBJ_DIR) $(TARGET)

$(OBJ_DIR):
	#mkdir -p $@
	
$(TARGET): $(addprefix $(OBJ_DIR), $(OBJ))
	$(CC) $(LDFLAGS) $(addprefix $(OBJ_DIR), $(OBJ)) -o $@
	
$(OBJ_DIR)%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	#rm -rf ./obj/
	rm $(addprefix $(OBJ_DIR), $(MAIN))
	


#
#
#MAIN_OBJ = main.o mode.o threads.o sensor.o
#SENSR_OBJ = adConvert.o
#GPIO_OBJ = gpio.o gpio-spi.o gpio-util.o gpio-timer.o gpio-i2c.o gpio-arm-timer.o
#	
#
#
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
