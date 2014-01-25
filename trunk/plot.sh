#!/bin/sh

yday=`date +%F -d '1 day ago'`
#echo $yday
Rscript logPlot.r $yday png "/var/log/cas/"
