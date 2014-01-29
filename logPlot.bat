@echo off
set day=%~n1
set r="Rscript.exe"
set current="./"
set opt=%~dp0logPlot.r %day% png %current% correct

%r% %opt%
