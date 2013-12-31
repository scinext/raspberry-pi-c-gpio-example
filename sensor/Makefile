CC = gcc

#コンパイル時のフラグ -g gdbでのデバッグ用 -lrt(getclock_) -lm(math.h)
CFLAGS =-g -O3
#リンク時のフラグ
LDFLAGS =-O3 -lrt -lm 

#一時ファイルの出力場所
OBJ_DIR = ./obj/

#コンパイルファイル
GIPO_DIR = gpio/
SRC += $(addprefix $(GIPO_DIR), gpio.c gpio-spi.c gpio-util.c gpio-timer.c gpio-i2c.c gpio-arm-timer.c)
SRC += main.c adConvert.c sensor.c

OBJ=$(addprefix $(OBJ_DIR), $(SRC:.c=.o) )

#実行ファイル名
TARGET=sensor

all: $(TARGET)

$(TARGET): $(OBJ)
	#$<, $@
	$(CC) $(LDFLAGS) $(OBJ) -o $@
	
	#sudo ./sensor

#$(OBJ)が$(OBJ_DIR)%.oのパターンに引っかかるのでここが適用される
#パターンの%が使えるので%.cの依存関係が確立してmakeされる
#作成するオブジェクトのディレクトリを取得してmkdir
$(OBJ_DIR)%.o : %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OJB_DIR)

clean:
	rm ../obj/*.o
	
	
#Make書式 http://www.unixuser.org/~euske/doc/makefile/
#依存関係とコマンドとの見分けは行頭にタブがあるかないかのみ
#ソースを省略するとタイムスタンプ比較ができないため必ず実行する
#ソースが更新されていたらコマンドを実行する
#ソースが他から生成されたものなら先にそちらを行う
#
#ターゲット: ソース
#	コマンド
#
#サフィックスルール
#	.suffix1.suffix2
#		command1
#	suffix2のファイルがsuffix1から作成できる(suffix1 -> suffix2)
#
#
#関数(textは基本的に空白区切り)
#http://www.ecoop.net/coop/translated/GNUMake3.77/make_toc.jp.html
#
#$(patsubst pattern, replace, text)
#	textをパターンにヒットした部分を置き換える
#	pattern内%->ワイルドカード
#	replace内%->上記でヒットした部分
#
#	$(patsubst %.c, %.o, bar.c foo.c)
#		bar.o foo.o	
#
#代用参照
#	変数を利用するときに末尾が指定したものだった場合置き換える
#	patsubstよりも場合によっては楽
#	foo = a.o b.o c.o
#	bar = $(foo:.o=.c)
#		a.c b.c c.c
#
#
#$(addprefix prefix, text)
#	textの先頭にprefixをつける
#	$(addprefix src/, foo bar)
#		src/foo src/bar
#
#$(notdir text)
#	textからディレクトリを削除する
#	$(notdir dir/foo.c bar.c)
#		foo.c bar.c
#
#$(dir text)
#	textからディレクトリのみにするディレクトリがない場合は'./'になる
#	$(dir src/foo.c bar.c)
#		src/ ./
#
#
#
#
#
#
#
#
