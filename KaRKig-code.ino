#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32_TANK";
const char* password = "12345678";

WebServer server(80);

// L298N 
#define ENA 25
#define IN1 26
#define IN2 27
#define ENB 14
#define IN3 12
#define IN4 13

// PWM 
const int freq = 1000;
const int resolution = 8;

int speedValue = 170;
int moveX = 0;
int moveY = 0;

// WEB PAGE
const char MAIN_page[] PROGMEM = R"=====(

<!DOCTYPE html>

<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">

<style>

*{
-webkit-user-select:none;
user-select:none;
-webkit-touch-callout:none;
-webkit-tap-highlight-color:transparent;
}

body{
margin:0;
background:linear-gradient(160deg,#0b0f14,#111927);
color:white;
font-family:Arial;
overflow:hidden;
}

.title{
text-align:center;
font-size:24px;
padding:8px;
letter-spacing:2px;
}

.container{
display:grid;
grid-template-columns:1fr 1fr 1fr;
align-items:center;
height:92vh;
padding:10px;
}

.left{
display:flex;
flex-direction:column;
gap:45px;
align-items:center;
}

.right{
display:flex;
gap:45px;
align-items:center;
justify-content:center;
}

.btn{
width:130px;
height:130px;
border-radius:28px;
border:none;
font-size:32px;
font-weight:bold;
color:white;
background:linear-gradient(145deg,#1c2530,#0e141b);
box-shadow:0 6px 18px rgba(0,0,0,0.8), inset 0 0 18px rgba(0,0,0,0.9);
transition:0.08s;
touch-action:manipulation;
}

.btn:active{
transform:scale(0.92);
background:linear-gradient(145deg,#3aa0ff,#1f6dff);
box-shadow:0 0 25px #2f8cff, inset 0 0 12px #ffffff40;
}

.center{
display:flex;
flex-direction:column;
align-items:center;
gap:12px;
}

#speed{
width:280px;
height:40px;
touch-action:auto;
}

.stop{
margin-top:8px;
width:200px;
height:95px;
font-size:30px;
border-radius:25px;
border:none;
color:white;
background:linear-gradient(145deg,#ff3b3b,#a80000);
box-shadow:0 0 30px red;
touch-action:manipulation;
}

.stop:active{
transform:scale(0.9);
box-shadow:0 0 50px red;
}

</style>

</head>

<body>

<div class="title">ESP32 TANK</div>

<div class="container">

<div class="left">
<button class="btn up">▲</button>
<button class="btn down">▼</button>
</div>

<div class="center">
<div>SPEED</div>
<input type="range" min="80" max="255" value="170" id="speed">
<button class="stop">STOP</button>
</div>

<div class="right">
<button class="btn leftb">◀️</button>
<button class="btn rightb">▶️</button>
</div>

</div>

<script>

let state={up:false,down:false,left:false,right:false};
let timer=null;

function send(){
let y=0,x=0;
if(state.up) y=1;
if(state.down) y=-1;
if(state.left) x=-1;
if(state.right) x=1;
fetch(⁠ /move?y=${y}&x=${x} ⁠);
}

function start(){
if(timer===null){
timer=setInterval(send,100);
}
}

function stop(){
if(timer!==null){
clearInterval(timer);
timer=null;
fetch(⁠ /move?y=0&x=0 ⁠);
}
}

function bind(selector,key){
let b=document.querySelector(selector);

b.addEventListener("pointerdown",e=>{
e.preventDefault();
b.setPointerCapture(e.pointerId);
state[key]=true;
start();
});

b.addEventListener("pointerup",e=>{
state[key]=false;
});

b.addEventListener("pointercancel",()=>state[key]=false);
b.addEventListener("pointerleave",()=>state[key]=false);
}

bind(".up","up");
bind(".down","down");
bind(".leftb","left");
bind(".rightb","right");

// STOP
document.querySelector(".stop").addEventListener("pointerdown",()=>{
state={up:false,down:false,left:false,right:false};
stop();
fetch(⁠ /stop ⁠);
});

// скорость
document.getElementById("speed").addEventListener("input",e=>{
fetch(⁠ /speed?val=${e.target.value} ⁠);
});

document.body.addEventListener("contextmenu",e=>e.preventDefault());

</script>

</body>
</html>
)=====";

// MOTOR 
void setMotor(int left, int right){

if(left>0){ digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW); }
else if(left<0){ digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH); left=-left; }
else{ digitalWrite(IN1,LOW); digitalWrite(IN2,LOW); }

if(right>0){ digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW); }
else if(right<0){ digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH); right=-right; }
else{ digitalWrite(IN3,LOW); digitalWrite(IN4,LOW); }

left=constrain(left,0,255);
right=constrain(right,0,255);

ledcWrite(ENA,left);
ledcWrite(ENB,right);
}

//  HANDLERS 
void handleMove(){
moveY=server.arg("y").toInt();
moveX=server.arg("x").toInt();

int forward=moveY*speedValue;
int turn=moveX*speedValue;

int leftMotor=constrain(forward+turn,-255,255);
int rightMotor=constrain(forward-turn,-255,255);

setMotor(leftMotor,rightMotor);
server.send(200,"text/plain","OK");
}

void handleSpeed(){
speedValue=server.arg("val").toInt();
server.send(200,"text/plain","OK");
}

void handleStop(){
setMotor(0,0);
server.send(200,"text/plain","STOP");
}

void handleRoot(){
server.send_P(200,"text/html",MAIN_page);
}

// SETUP 
void setup(){

Serial.begin(115200);

pinMode(IN1,OUTPUT);
pinMode(IN2,OUTPUT);
pinMode(IN3,OUTPUT);
pinMode(IN4,OUTPUT);

ledcAttach(ENA,freq,resolution);
ledcAttach(ENB,freq,resolution);

WiFi.softAP(ssid,password);

Serial.println("READY");
Serial.println(WiFi.softAPIP());

server.on("/",handleRoot);
server.on("/move",handleMove);
server.on("/speed",handleSpeed);
server.on("/stop",handleStop);

server.begin();
}

void loop(){
server.handleClient();
}
