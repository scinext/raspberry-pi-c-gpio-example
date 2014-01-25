
file <- "file path"
dat <- read.table(file, header=TRUE, sep="\t", skip=1)

D <- "time"
T <- "Temp"
P <- "Press"
L <- "Lux"
H <- "Humidity"

#ƒ}[ƒWƒ“
par(mar = c(4, 18, 3, 1))

postscript("test.eps", horizontal=FALSE, height=9, width=14, pointsize=15)
colors	<- c("", "red", "green", "orange", "blue")
lines	<- c(0, 1.5, 5.5, 9.5, 13.5)
lineName <- c("", "Temp[C]", "Pressure[hPa]", "Lux", "Humidity[%]")

for(i in 2:ncol(dat))
{
	list <- dat[ , i]
	plot(list , type="l", xaxt="n", xaxs="i", yaxt="n", yaxs="r", xlab="", ylab="", col=colors[i])
	#cŽ² seq by‚Í‚Ý’l len‚ÍŒÂ”
	hLine <- round(seq(min(list), max(list), len=10), 1)
	axis(side=2, at=c(hLine), labels=c(hLine), line=lines[i], col=colors[i])
	mtext(side=2, text=lineName[i], line=lines[i]-1, col=colors[i])
	#abline(h=c(hLine), col="gray", lty=2, lwd=1)
	par(new = TRUE)
}

#ŽžŠÔŽ²‚Ì•`‰æ
vLine <- seq(1, length(dat[, D]), by=10)
axis(side=1, at=c(vLine), labels=dat[c(vLine) , D] )
abline(v=dat[c(vLine), D], col="gray", lty=2, lwd=1)
mtext(side=1, text=D, line=2)
#mtext(side=1, text=D, line=1, at=c(par()$usr[1]-12) )

#˜g‚Ì•`‰æ
box()

#ƒ^ƒCƒgƒ‹
revers	<- rev( colnames(dat[2: ncol(dat)]) )
t	<- paste(revers, collapse=", " )
title( t )

dev.off()
