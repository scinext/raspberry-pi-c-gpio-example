
args	<- commandArgs(TRUE)
date	<- args[1]
pct		<- args[2]
current <- args[3]	#指定箇所にあるログから画像の作成を行う

#空の場合はnullでなくnaで判定
if( is.na(date) )
	date <- "2014-01-19"
file <- sprintf("%s%s.log", current, date)
dat <- read.table(file, header=TRUE, sep="\t", skip=1)
pngFile <- sprintf("%s%s.png", current, date)

#アクセス方法
#x[2, ]	2行目を取り出す
#x[ ,2]	2列目を取り出す
#x[1, 2]	1行 2列目の成分を取り出す
#x[c(1,2), 2]		1,2行 2列目の成分を取り出す
#x[c(1,2), c(1,3)]	1,2行目と 1,3列を取り出す
#summary(dat)

D <- "time"
T <- "Temp"
P <- "Press"
L <- "Lux"
H <- "Humidity"

#matplot( dat[ , T], pch=1:4, type="l")
#x <- dat[ ,c(D, T)]; names(x) <- c(D, T)
#plot(x)
#plot( dat[c(1,2)], type="l")
#axes=FALSE 軸の生成 x[y]axt="n", x[y]axs="i"グラフの書き方 r余白あり iなし
#mtext("time", side=1)
#ylim=c(y最少, y最大)

#pngの作成開始
if( !is.na(pct) )
	png(pngFile, width=853, height=480)

#マージン
par(mar = c(4, 18, 3, 1))

colors	<- c("", "red", "green", "orange", "blue")
lines	<- c(0, 1.5, 5.5, 9.5, 13.5)
lineName <- c("", "Temp[C]", "Pressure[hPa]", "Lux", "Humidity[%]")

for(i in 2:ncol(dat))
{
	list <- dat[ , i]
	plot(list , type="l", xaxt="n", xaxs="i", yaxt="n", yaxs="r", xlab="", ylab="", col=colors[i])
	#縦軸 seq byは刻み値 lenは個数
	hLine <- round(seq(min(list), max(list), len=10), 1)
	axis(side=2, at=c(hLine), labels=c(hLine), line=lines[i], col=colors[i])
	mtext(side=2, text=lineName[i], line=lines[i]-1, col=colors[i])
	#abline(h=c(hLine), col="gray", lty=2, lwd=1)
	par(new = TRUE)
}

#時間軸の描画
vLine <- seq(1, length(dat[, D]), by=10)
axis(side=1, at=c(vLine), labels=dat[c(vLine) , D] )
abline(v=dat[c(vLine), D], col="gray", lty=2, lwd=1)
mtext(side=1, text=D, line=2)
#mtext(side=1, text=D, line=1, at=c(par()$usr[1]-12) )

#枠の描画
box()

#凡例
#par()$usr グラフの四隅位置が入る x1, x2, y1, y2
#ncol=ncol(dat)-1 列数
#lx <- par()$usr[2]
#ly <- par()$usr[4] - ((par()$usr[4] - par()$usr[3]) / 2)
#par(xpd=TRUE)
#legend(lx, ly, legend=lineName[2: ncol(dat)], col=colors[2: ncol(dat)], lty=1)
#par(xpd=FALSE)

#タイトル
revers	<- rev( colnames(dat[2: ncol(dat)]) )
t	<- paste(revers, collapse=", " )
title( t )

#png終了
if( !is.na(pct) )
	dev.off()