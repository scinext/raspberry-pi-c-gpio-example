
//use putty & R
var putty	= "pscp.exe "
var rpi		= "-pw raspberry pi@[ password ]:"
var casLog	= "/var/log/cas/"
var ext		= ".log"
var saveDir	= " ./"
var R		= "Rscript.exe "
var rscript = "logPlot.r "
var imgExt	= " png"
var correct = " correct"


var t	= new Date();
var toDay = t.getFullYear()+'-'+('0'+(t.getMonth()+1)).slice(-2)+'-'+('0'+t.getDate()).slice(-2);

var y	= new Date();
y.setDate( y.getDate()-1 );
var yesterDay = y.getFullYear()+'-'+('0'+(y.getMonth()+1)).slice(-2)+'-'+('0'+y.getDate()).slice(-2);

//WScript.Echo(toDay);
//WScript.Echo(yesterDay);

var shell	= new ActiveXObject("WScript.Shell");
var execCmd;

execCmd = putty+rpi+casLog+yesterDay+ext+saveDir;
shell.Exec( execCmd );

execCmd = putty+rpi+casLog+toDay+ext+saveDir;
shell.Exec( execCmd );


WScript.sleep(10000);

execCmd = R+shell.CurrentDirectory+'\\'+rscript+yesterDay+imgExt+saveDir+correct
shell.Exec( execCmd );

execCmd = R+shell.CurrentDirectory+'\\'+rscript+toDay+imgExt+saveDir+correct
shell.Exec( execCmd );
