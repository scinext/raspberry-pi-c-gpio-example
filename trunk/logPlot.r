
args	<- commandArgs(TRUE)
date	<- args[1]  #日付
pct		<- args[2]	#画像の保存 何かデータがあればpngで出力
current <- args[3]	#指定箇所にあるログから画像の作成を行う
correct <- args[4]  #補正を行うかどうか行う場合はXXX-correct.logで保存
#correct<- "currect"
#date	<- "2014-01-26"
#pct		<- NA
#current	<- "z:\\"
#date	<- NA
	
file <- sprintf("%s%s.log", current, date)
dat <- read.table(file, header=TRUE, sep="\t", skip=1)
pngFile <- sprintf("%s%s.png", current, date)
if( !is.na(correct) )
	correctionFile <- sprintf("%s%s-correct.log", current, date)

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


#海面更正時の各種データ
z			<- 27		#標高
elr			<- 0.0065	#気温減率 0.65℃/100m 気象庁だと0.5℃/100m
seaTempDiff	<- z * elr	#海面気温との差
atmos		<- 1013.25	#標準気圧

#空の場合はnullでなくnaで判定
if( !is.na(correct) )
{
	# 気象観測の手引き http://www.jma.go.jp/jma/kishou/know/kansoku_guide/tebiki.pdf
	#P0 = P+P(e^(gz/RTvm)-1)
	#R 乾燥空気の気体定数 287.05
	#Tvm(K) Tvm= 273.15+tm+em
	#tm 気柱の平均気温 tm= t+0.005*z/2 = t+0.0025z
	#em 湿度による補正 em= Atm^2+Btm+C
	#		温度範囲		A			B			C
	#		tm	<-30		0			0			0.090
	#	-30 <=	tm	<0		0.000489	0.03		0.550
	#	0   <=	tm	<20		0.00285		0.0165		0.550
	#	20  <=	tm	<33.8	-0.006933	0.4687		-4.58
	#	33.8<=	tm			0			0			3.34
	gasConstant	<- 287.05	#乾燥空気の気体定数
	gravity		<- 9.7978	#重力加速度(気象庁のデータより)
	elr2		<- 0.005	#気温減率 (気象庁のデータより)
	tm <- dat[ , T]+(elr2*z/2)
	em <- NULL
	for(item in tm )
	{
		if( item < -30 )
			emtmp <- 0.09
		else if( item < 0 )
			emtmp <- 0.000489*item^2 + 0.03*item + 0.55
		else if( item < 20 )
			emtmp <- 0.00285*item^2 + 0.0165*item + 0.55
		else if( item < 33.8 )
			emtmp <- -0.006933*item^2 + 0.4687*item + -4.58
		else
			emtmp <- 3.34
		em <- c( em, emtmp )
	}
}

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
	if( colnames(dat)[i] == "Press" && !is.na(correct) ) {
		#海面更正は気象庁の手引きに従う
		# http://www.es.ris.ac.jp/~nakagawa/met_cal/sea_press.html
		# p0=p(1-0.0065*z/(t+0.0065z+273.15))^-5.257
		#list <- round( 
		#	dat[ , i]*(1-seaTempDiff/
		#	(dat[ , T]+seaTempDiff+273.15))^-5.257, 2)

		#気象庁の海面更正
		#P0 = P+P(e^(gz/RTvm)-1)
		list	<- round( 
				dat[ , i]+dat[ , i]*(
					exp( (gravity*z)/( gasConstant*(273.15+tm+em) ))-1
				), 2)

		# 高度 http://keisan.casio.jp/has10/SpecExec.cgi?id=system/2006/1257609530 内コメント
		# 高度は海面が1気圧(1013.25)の時を想定する
		# h=(1013.25/P0)^(1/5.257)*((P0/P)^(1/5.257)-1)*(T+273.15)/0.0065
		alti <- round( 
			(atmos/atmos)^(1/5.257)*( ((atmos/dat[ , i])^(1/5.257))-1 )*(dat[ , T]+273.15)/
			elr, 2)

		#列を追加して補正したlogを後で出力する
		correctData <- cbind(dat, list)
		colnames(correctData)[ ncol(correctData) ]  <- "sea level press"
		correctData <- cbind(correctData , alti)
		colnames(correctData)[ ncol(correctData) ]  <- "altitude"
	} else {
		list <- dat[ , i]
	}
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

#補正したデータの出力 print(correctionFile)
if( !is.na(correct) )
	write.table(correctData, correctionFile, quote=F, sep="\t", row.names=F)