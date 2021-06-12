#include <ESP8266WebServer.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "AnotherIFTTTWebhook.h"
#include <math.h>

#define DELAY_MS 5500
#define DHT11PIN 10
#define LED1 0
#define LED2 14
#define LED3 12

//MPU6050 관련 변수 (칼만필터)
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

int MPU_Address=0x68;

double angleAcX, angleAcY, angleAcZ;
double angleGyX, angleGyY, angleGyZ;
double angleFiX, angleFiY, angleFiZ;

const double RADIAN_TO_DEGREE = 180 / 3.14159;
const double DEGREE_PER_SECOND = 32767 / 250;
const double ALPHA = 1 / (1 + 0.04);
double Detection;

unsigned long now = 0;
unsigned long past = 0;
double dt = 0;

double baseAcX, baseAcY, baseAcZ;
double baseGyX, baseGyY, baseGyZ;
double House_avg;

char Angle[10];
char House_angle[10];
char Earthquake_Detection[10];

////////////////////////////////////////////////////////////////////////

//DHT11 관련 변수
int readTmp;
int readHumid;

char out_temp[10];
char in_temp[10];
char in_hum[10];

char Distance_b[10];
long duration, distance; 
////////////////////////////////////////////////////////

//SR-04 관련 변수
unsigned long echo_time = 0;

int trigPin=13; //초음파 센서 trig
int echoPin=15;  //초음파 센서 echo

////////////////////////////////////////////////
HTTPClient myclient;
DynamicJsonDocument doc(2048);
ESP8266WebServer myHTTPServer(80);//포트번호
unsigned long long lastMs=0;
unsigned long long loopMs=0;

// ///////////////////////메인 페이지 관련 함수/////////////////////////////////////
void fnRoot(void)
{
  char tmpBuffer[2000];
  strcpy(tmpBuffer, "<html>\r\n");
  strcat(tmpBuffer, "IOT Smart Home System Project<br><br>\r\n");
  strcat(tmpBuffer, "<a href=/status>Weather</a><br><br>\r\n");
  strcat(tmpBuffer, "<a href=/house>House_status</a><br><br>\r\n");
  strcat(tmpBuffer, "<a href=/ECHO>Ultrasonic Sensor</a><br><br>\r\n");
  strcat(tmpBuffer, "<a href=/LIGHT>Main Light Controller</a><br><br>\r\n");
  strcat(tmpBuffer, "<a href=/EARTHQUAKE>EARTHQUAKE_Alert</a><br><br>\r\n");
  snprintf(tmpBuffer, sizeof(tmpBuffer), "%s%s", tmpBuffer, "</html>");
  
  myHTTPServer.send(200, "text/html", tmpBuffer); 
}

///////////////////////Room on/0ff 관련 함수
void Room1(void){
  
  char room_1[500];
  strcpy(room_1, "<meta charset=utf-8>");
  strcat(room_1, "Room 1 Light<br><br>");
  strcat(room_1, "<a href=/Room1_on>room1_on</a><br><br>\r\n");
  strcat(room_1, "<a href=/Room1_off>room1_off</a><br><br>\r\n");
  strcat(room_1, "<a href=/>Go back to Main page</a><br>\r\n");
  strcat(room_1, "<a href=/LIGHT>Go back to Light Controller</a><br>\r\n");
  myHTTPServer.send(200, "text/html", room_1);
  
  }
  
void Room2(void){
  
  char room_2[500];
  strcpy(room_2, "<meta charset=utf-8>");
  strcat(room_2, "Room 2 Light<br><br>");
  strcat(room_2, "<a href=/Room2_on>room2_on</a><br><br>\r\n");
  strcat(room_2, "<a href=/Room2_off>room2_off</a><br><br>\r\n");
  strcat(room_2, "<a href=/>Go back to Main page</a><br>\r\n");
  strcat(room_2, "<a href=/LIGHT>Go back to Light Controller</a><br>\r\n");
  myHTTPServer.send(200, "text/html", room_2);
  
  }
  
void Room3(void){
  
  char room_3[500];
  strcpy(room_3, "<meta charset=utf-8>");
  strcat(room_3, "Room 3 Light<br><br>");
  strcat(room_3, "<a href=/Room3_on>room3_on</a><br><br>\r\n");
  strcat(room_3, "<a href=/Room3_off>room3_off</a><br><br>\r\n");
  strcat(room_3, "<a href=/>Go back to Main page</a><br>\r\n");
  strcat(room_3, "<a href=/LIGHT>Go back to Light Controller</a><br>\r\n");
  myHTTPServer.send(200, "text/html", room_3);
  
  }

////////////////LED ON///////////////////////////////////
void Room1_on(void){

  char room1_on[500];
  strcpy(room1_on, "<meta charset=utf-8>");
  strcat(room1_on, "Room2 Light ON");
  strcat(room1_on, "<br><br>");
  strcat(room1_on, "<a href=/ROOM1>Go back to Room1 Controller</a><br>\r\n");
  strcat(room1_on, "<a href=/LIGHT>Go back to Main Light Controller</a><br>\r\n");
  strcat(room1_on, "<a href=/>Go back to Main page</a><br>\r\n");
  
  digitalWrite(LED1, HIGH);
  
  myHTTPServer.send(200, "text/html", room1_on);
  
  }

///////////////////LED OFF/////////////////////////////////////////////
void Room1_off(void){
  
  char room1_off[500];
  strcpy(room1_off, "<meta charset=utf-8>");
  strcat(room1_off, "Room2 Light OFF");
  strcat(room1_off, "<br><br>");
  strcat(room1_off, "<a href=/ROOM1>Go back to Room1 Controller</a><br>\r\n");
  strcat(room1_off, "<a href=/LIGHT>Go back to Main Light Controller</a><br>\r\n");
  strcat(room1_off, "<a href=/>Go back to Main page</a><br>\r\n");
  
  digitalWrite(LED1, LOW);
  myHTTPServer.send(200, "text/html", room1_off);
  
  }

void Room2_on(void){

  char room2_on[500];
  strcpy(room2_on, "<meta charset=utf-8>");
  strcat(room2_on, "Room2 Light ON");
  strcat(room2_on, "<br><br>");
  strcat(room2_on, "<a href=/ROOM2>Go back to Room2 Controller</a><br>\r\n");
  strcat(room2_on, "<a href=/LIGHT>Go back to Main Light Controller</a><br>\r\n");
  strcat(room2_on, "<a href=/>Go back to Main page</a><br>\r\n");
  
  digitalWrite(LED2, HIGH);
  
  myHTTPServer.send(200, "text/html", room2_on);
  
  }

void Room2_off(void){
  
  char room2_off[500];
  strcpy(room2_off, "<meta charset=utf-8>");
  strcat(room2_off, "Room2 Light OFF");
  strcat(room2_off, "<br><br>");
  strcat(room2_off, "<a href=/ROOM2>Go back to Room2 Controller</a><br>\r\n");
  strcat(room2_off, "<a href=/LIGHT>Go back to Main Light Controller</a><br>\r\n");
  strcat(room2_off, "<a href=/>Go back to Main page</a><br>\r\n");
  
  digitalWrite(LED2, LOW);
  myHTTPServer.send(200, "text/html", room2_off);
  
  }

void Room3_on(void){
  
  char room3_on[500];
  strcpy(room3_on, "<meta charset=utf-8>");
  strcat(room3_on, "Room3 Light ON");
  strcat(room3_on, "<br><br>");
  strcat(room3_on, "<a href=/ROOM3>Go back to Room3 Controller</a><br>\r\n");
  strcat(room3_on, "<a href=/LIGHT>Go back to Main Light Controller</a><br>\r\n");
  strcat(room3_on, "<a href=/>Go back to Main page</a><br>\r\n");
  
  digitalWrite(LED3, HIGH);
  myHTTPServer.send(200, "text/html", room3_on);
  
  }

void Room3_off(void){
  
  char room3_off[500];
  strcpy(room3_off, "<meta charset=utf-8>");
  strcat(room3_off, "Room3 Light OFF");
  strcat(room3_off, "<br><br>");
  strcat(room3_off, "<a href=/ROOM3>Go back to Room3 Controller</a><br>\r\n");
  strcat(room3_off, "<a href=/LIGHT>Go back to Main Light Controller</a><br>\r\n");
  strcat(room3_off, "<a href=/>Go back to Main page</a><br>\r\n");
  
  digitalWrite(LED3, LOW);
  
  myHTTPServer.send(200, "text/html", room3_off);

  
  }

///////////////////////////Main Light Controller 페이지////////////////////////////
void LIGHT_Controller (void){
  
    char LIGHT_Control[500];
    strcpy(LIGHT_Control, "<meta charset=utf-8>");
    strcat(LIGHT_Control, "Main Light Controller<br><br>");
    strcat(LIGHT_Control, "조명을 킬 방을 고르시오<br><br>");
    
    strcat(LIGHT_Control, "<a href=/ROOM1>Room1</a><br><br>\r\n");
    strcat(LIGHT_Control, "<a href=/ROOM2>Room2</a><br><br>\r\n");
    strcat(LIGHT_Control, "<a href=/ROOM3>Room3</a><br><br>\r\n");
    strcat(LIGHT_Control, "<a href=/>Go back to Main page</a><br>\r\n");
    
    myHTTPServer.send(200, "text/html", LIGHT_Control);
  }

////////////////////////////////SR-04 측정 함수////////////////////////////////
void Dis()
{
  if (millis() - loopMs>150)
  {
    loopMs=millis();
    //Trig 핀으로 10us의 pulse 발생
    digitalWrite(trigPin, LOW);        //Trig 핀 Low
    delayMicroseconds(2);            //2us 유지
    digitalWrite(trigPin, HIGH);    //Trig 핀 High
    delayMicroseconds(10);            //10us 유지
    digitalWrite(trigPin, LOW);        //Trig 핀 Low
    
    //Echo 핀으로 들어오는 펄스의 시간 측정
    duration = pulseIn(echoPin, HIGH);        //pulseIn함수가 호출되고 펄스가 입력될 때까지의 시간. us단위로 값을 리턴.
    
    //음파가 반사된 시간을 거리로 환산
    //음파의 속도는 340m/s 이므로 1cm를 이동하는데 약 29us.
    //따라서, 음파의 이동거리 = 왕복시간 / 1cm 이동 시간 / 2 이다.
    distance = duration / 29 / 2;        //센치미터로 환산  
  }
}

///////////////////////////////Ultrasonic Sensor 페이지///////////////////////////////
void ECHO(void){
    char ECHO_string[400];
    Dis();
    sprintf(Distance_b, "%d",distance);

    strcpy(ECHO_string, "<meta charset=utf-8>");
    strcat(ECHO_string, "센서에 포착된 사람과의 거리: ");
    strcat(ECHO_string, Distance_b);
    strcat(ECHO_string, " cm");
    strcat(ECHO_string, "<br>");
    myHTTPServer.send(200, "text/html", ECHO_string);
    
    if(distance<80)//default 값보다 가까울 때 
    {
      send_webhook("Door","cLRy-Rn7OL62uIGLLVXvVc","","","");
    }

    
}

//////////////////// mpu6050 값 받기///////////////////////////////
void getData() {
  Wire.beginTransmission(MPU_Address);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_Address, 14, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  Tmp = Wire.read() << 8 | Wire.read();
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();
} // mpu6050 값 받기

/////////////////////////////칼만 필터//////////////////////
void getDT() {
  now = millis();
  dt = (now - past) / 1000.0;
  past = now;
}

void calibrateSensor() {
  double sumAcX = 0, sumAcY = 0, sumAcZ = 0;
  double sumGyX = 0, sumGyY = 0, sumGyZ = 0;
  getData();
  for (int i = 0; i < 1000 ; i++) {
    getData();
    sumAcX += AcX; sumAcY += AcY; sumAcZ += AcZ;
    sumGyX += GyX; sumGyY += GyY; sumGyZ += GyZ;
    delay(10);
  }
  baseAcX = sumAcX / 1000;
  baseAcY = sumAcY / 1000;
  baseAcZ = sumAcZ / 1000;
  baseGyX = sumGyX / 1000;
  baseGyY = sumGyY / 1000;
  baseGyZ = sumGyZ / 1000;
}

void Kalman() //kalman filter
{
  getData();
  getDT();

  angleAcY = atan(-AcX / sqrt(pow(AcY, 2) + pow(AcZ, 2)));
  angleAcY *= RADIAN_TO_DEGREE;

  angleGyY += ((GyY - baseGyY) / DEGREE_PER_SECOND) * dt;

  double angleTmp = angleFiY + angleGyY * dt;
  angleFiY = ALPHA * angleTmp + (1.0 - ALPHA) * angleAcY; //집의 현재 기울기
  //Serial.println(angleFiY);
}

/////////////////////EARTHQUAKE Alert 페이지////////////////////////////////////////
void MPU6050(void)
  {
    char MPU6050_string[400];

    sprintf(Angle, "%f",angleFiY); 
    sprintf(House_angle, "%f", House_avg);
    sprintf(Earthquake_Detection, "%f", Detection);
    
    strcpy(MPU6050_string, "<meta charset=utf-8>");
    strcat(MPU6050_string, "집의 평균 상태: ");
    strcat(MPU6050_string, House_angle);
    strcat(MPU6050_string, "<br>");
    
    strcat(MPU6050_string, "현재 집의 상태: ");
    strcat(MPU6050_string, Angle);
    strcat(MPU6050_string, "<br>");
    
    strcat(MPU6050_string, "현재 집의 기울어진 정도: ");
    strcat(MPU6050_string, Earthquake_Detection);
    strcat(MPU6050_string, "<br>");

    if(Detection>5)
    {
      strcat(MPU6050_string, "집이 지진 발생!");
      strcat(MPU6050_string, "<br>");
      send_webhook("Earthquake","cLRy-Rn7OL62uIGLLVXvVc","","","");
    }
    else
    {
      strcat(MPU6050_string, "현재 상태 양호!");
      strcat(MPU6050_string, "<br>");
    }
    myHTTPServer.send(200, "text/html", MPU6050_string);
 
  }

/////////////////////////// Weather page //////////////////////////////////////
void openweatherAPI(void){
    char weather_string[400];
    myclient.begin("http://api.openweathermap.org/data/2.5/weather?q=yongin&appid=30f77f296496bd2c44ef28d20211a938");
    
    if (myclient.GET()==HTTP_CODE_OK & millis()- lastMs >= DELAY_MS)
    {
      lastMs = millis();
      String recieveData = myclient.getString();
      //Serial.printf("%s\r\n\r\n END Transmission\r\n",recieveData.c_str());
      deserializeJson(doc,  recieveData);
      
      const char* whe = doc["weather"][0]["main"];
      String city = doc["name"];
      float tmp = doc["main"]["temp"];
  
      Serial.printf("현재 %s의 날씨는 %s 입니다.\r\n",city.c_str(),whe);
      Serial.printf("현재 온도는: %f 입니다.\r\n",(float)(doc["main"]["temp"]));
      //Serial.printf("습도는: %f 입니다.\r\n",(float)(doc["main"]["humidity"]));
      strcpy(weather_string, "<meta charset=utf-8>");
      strcat(weather_string, "현재 날씨: ");
      strcat(weather_string, whe);
      strcat(weather_string, "<br>");
      
      strcat(weather_string, "현재 온도: ");
      sprintf(out_temp, "%f",(float)tmp-273.0);
      strcat(weather_string, out_temp);
      strcat(weather_string, "  °C<br><br>");
      strcat(weather_string, "<a href=/>Go back to Main page</a><br>\r\n");
      myHTTPServer.send(200, "text/html", weather_string);
      printf("%s\r\n",whe);
      delay(3000);
      char* weather_c = (char*)whe;
      if(strcmp(weather_c,"Rain")==0)//Rain
        {
          send_webhook("Weather","cLRy-Rn7OL62uIGLLVXvVc","","","");
        }
        
    }
    else
    {
      Serial.printf("Server something wrong\r\n");  
    }
    
    

  }

////////////////////DHT11 값 받기 ////////////////////////////
int readDHT11(int *readTmp, int* readHumid)
{
  int dt[82]={0};
  //phase 1
  digitalWrite(DHT11PIN, 1);
  pinMode(DHT11PIN, OUTPUT);
  delay(1);
  digitalWrite(DHT11PIN, 0);
  delay(20);
  pinMode(DHT11PIN, INPUT_PULLUP);
  
  while(1)
  {
    if(digitalRead(DHT11PIN)==1)
      break;//dataline '1' 확인
  }
  while(1)
  {
    if(digitalRead(DHT11PIN)==0)
      break;//dataline 'ready' 확인
  }
   //phase2,3
   
  int cnt=0;
  for (cnt=0;cnt<41;cnt++)
  {
    dt[cnt*2]=micros();
    while(1)
      if(digitalRead(DHT11PIN)==1)break;
    dt[cnt*2]=micros()-dt[cnt*2]; 
  
    dt[cnt*2+1]=micros();
    while(1)
      if(digitalRead(DHT11PIN)==0)break;
    dt[cnt*2+1]=micros()-dt[cnt*2+1];     
  }
  
  //phase4
  for(cnt=0;cnt<41;cnt++)
    Serial.printf("cnt:%d, dt=[%d, %d]\r\n",cnt,dt[cnt*2],dt[cnt*2+1]);
    
  *readHumid=0;
  *readTmp=0;
  for(cnt=1;cnt<9;cnt++)
  {
    *readHumid=*readHumid<<1;
    if(dt[cnt*2+1]>49)
    {
      *readHumid=*readHumid+1;
    } 
    else
    {
      *readHumid=*readHumid+0;
    } 

  }
  for(cnt=17;cnt<25;cnt++)
  {
    *readTmp=*readTmp<<1;
    if(dt[cnt*2+1]>49)
    {
      *readTmp=*readTmp+1;
    } 
    else
    {
      *readTmp=*readTmp+0;
    }
  }
}

///////////////House Status 페이지////////////////////////
void house_status(void)
{
  if(millis()- lastMs >= DELAY_MS)
    {
      lastMs = millis();
      char house_status[400];
      readDHT11(&readTmp,&readHumid);
      Serial.printf("Temp:%d, Humid:%d\r\n",readTmp, readHumid);
      
      strcpy(house_status, "<meta charset=utf-8>");
      strcat(house_status, "집안 습도: ");
      sprintf(in_hum, "%d",readHumid);
      strcat(house_status, in_hum);
      
      strcat(house_status, " 집안 온도: ");
      sprintf(in_temp, "%d",readTmp);
      strcat(house_status, in_temp);
      strcat(house_status, "<br>");
      strcat(house_status, "<a href=/>Go back to Main page</a><br>\r\n");
      myHTTPServer.send(200, "text/html", house_status);
    }
    if(readHumid>60)
      analogWrite(2,950); //가습기 틀기(모터돌리기)  
    else
      analogWrite(2,0); //가습기 끄기(모터돌리기)
}
void setup() {
  
  Serial.begin(115200);

  pinMode(2, OUTPUT);    // 모터 핀
  pinMode(trigPin, OUTPUT);    //Trig 핀 output으로 세팅
  pinMode(echoPin, INPUT);    //Echo 핀 input으로 세팅
  pinMode(LED1, OUTPUT);      // LED1
  pinMode(LED2, OUTPUT);      // LED2 
  pinMode(LED3, OUTPUT);      // LED3 
  //pinMode(5, OUTPUT);
//--------------------------------------Wifi connection--------------------------------------------------------------------------------  
  WiFi.mode(WIFI_AP_STA);
  Serial.printf("WiFimode:%d\r\n",WiFi.getMode());
  WiFi.begin("wifitime","a5641078");
  //WiFi.begin("SK_WiFiGIGAEB90_2.4G","1903034280");
  Wire.begin(4,5);
  Wire.beginTransmission(MPU_Address);
  Wire.write(0x6B);
  Wire.write(0x01);
  Wire.endTransmission(true);
  
  while(1) //Wifi connection 
  {
    if(WiFi.waitForConnectResult()==WL_CONNECTED)
    {
      Serial.printf("IP: %s gateway:%s \r\n", WiFi.subnetMask().toString().c_str(), WiFi.gatewayIP().toString().c_str());
      break; 
    }
    else
      delay(100);
  }

  Serial.printf("Wifi Connected\r\n"); 
  Serial.println(WiFi.localIP());
//--------------------------------------Wifi connection--------------------------------------------------------------------------------

  myHTTPServer.on("/",fnRoot); //call back 함수 등록
  myHTTPServer.on("/status",openweatherAPI);
  myHTTPServer.on("/house",house_status);
  myHTTPServer.on("/EARTHQUAKE",MPU6050);
  myHTTPServer.on("/ECHO",ECHO);
  myHTTPServer.on("/LIGHT",LIGHT_Controller);
  
  myHTTPServer.on("/ROOM1",Room1);
  myHTTPServer.on("/ROOM2",Room2);
  myHTTPServer.on("/ROOM3",Room3);
  
  myHTTPServer.on("/Room1_on",Room1_on);
  myHTTPServer.on("/Room2_on",Room2_on);
  myHTTPServer.on("/Room3_on",Room3_on);

  myHTTPServer.on("/Room1_off",Room1_off);
  myHTTPServer.on("/Room2_off",Room2_off);
  myHTTPServer.on("/Room3_off",Room3_off);
  
  myHTTPServer.begin(); //웹 서버 오픈

  calibrateSensor();
  for(int a=0; a<1000; a++)
  {
    Kalman();
    House_avg+=angleFiY;     
  }
  House_avg=House_avg/1000; // 집의 원래 상태(평균 기울기 값)
  past = millis();

}


void loop() {
  Kalman();
  Detection = abs(House_avg - angleFiY);// IFTTT 나 다른 함수가 들어가면 값 에러
  Serial.printf("angle; %f, avg: %f, Detection:%f \r\n",angleFiY, House_avg, Detection);
  myHTTPServer.handleClient();
}
