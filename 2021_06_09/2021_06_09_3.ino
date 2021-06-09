#include <ESP8266WebServer.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "AnotherIFTTTWebhook.h"
#include <math.h>

#define DELAY_MS 5500
#define DHT11PIN 12

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

int readTmp;
int readHumid;

const char* whe;
//String city;
float tmp;
unsigned long weather_time = 0;

char out_temp[10];
char in_temp[10];
char in_hum[10];

char Angle[10];
char House_angle[10];
char Earthquake_Detection[10];

char Distance_b[10];
long duration, distance; 
unsigned long echo_time = 0;

int trigPin=13; //초음파 센서 trig
int echoPin=15;  //초음파 센서 echo
int count=0;


HTTPClient myclient;
DynamicJsonDocument doc(2048);
ESP8266WebServer myHTTPServer(80);//포트번호
unsigned long long lastMs=0;
unsigned long long loopMs=0;
//String str;

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
    if(distance < 30)
    {
      count+=1; 
      delay(10); 
    }
  }
}
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
    
    if(count>60000)//10분시상 감지시
    {
      send_webhook("Thief","cLRy-Rn7OL62uIGLLVXvVc","","","");
    }

    
}


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
}

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

void Kalman()
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

void fnRoot(void)
{
  char tmpBuffer[2000];
  strcpy(tmpBuffer, "<html>\r\n");
  strcat(tmpBuffer, "IOT Smart Home System Project<br><br>\r\n");
  strcat(tmpBuffer, "<a href=/status>Weather</a><br>\r\n");
  strcat(tmpBuffer, "<a href=/house>House_status</a><br>\r\n");
  strcat(tmpBuffer, "<a href=/ECHO>Ultrasonic Sensor</a><br>\r\n");
  strcat(tmpBuffer, "<a href=/EARTHQUAKE>EARTHQUAKE_Alert</a><br>\r\n");
  
  
  
  //strcat(tmpBuffer, "<a href=/house>House_status</a><br>\r\n");
  snprintf(tmpBuffer, sizeof(tmpBuffer), "%s%s", tmpBuffer, "</html>");
  
  myHTTPServer.send(200, "text/html", tmpBuffer); 
}

void Weather_status()
{
  myclient.begin("http://api.openweathermap.org/data/2.5/weather?q=yongin&appid=30f77f296496bd2c44ef28d20211a938");
  
  if (myclient.GET()==HTTP_CODE_OK & millis()- lastMs >= DELAY_MS)
  {
    lastMs = millis();
    String recieveData = myclient.getString();
    //Serial.printf("%s\r\n\r\n END Transmission\r\n",recieveData.c_str());
    deserializeJson(doc,  recieveData);
    
    whe = doc["weather"][0]["main"];
    String city = doc["name"];
    tmp = doc["main"]["temp"];

    //Serial.printf("현재 %s의 날씨는 %s 입니다.\r\n",city.c_str(),whe);
    //Serial.printf("현재 온도는: %f 입니다.\r\n",(float)(doc["main"]["temp"]));  
  }  
}

void openweatherAPI(void){
    char weather_string[400];
    Weather_status(); 
    strcpy(weather_string, "<meta charset=utf-8>");
    strcat(weather_string, "현재 날씨: ");
    strcat(weather_string, whe);
    strcat(weather_string, "<br>");
    
    strcat(weather_string, "현재 온도: ");
    sprintf(out_temp, "%f",(float)tmp-273.0);
    strcat(weather_string, out_temp);
    myHTTPServer.send(200, "text/html", weather_string);
    
    if(whe=="Rain" & millis()-weather_time > 600000)
    {
      weather_time = millis();
      digitalWrite(16, 0); //led로 비 경고
      send_webhook("Weather","cLRy-Rn7OL62uIGLLVXvVc","","","");
    }
    else
      digitalWrite(16, 1);    
  }

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
      
      strcat(house_status, "집안 온도: ");
      sprintf(in_temp, "%d",readTmp);
      strcat(house_status, in_temp);
      myHTTPServer.send(200, "text/html", house_status);
    }
    if(readHumid>55)
      analogWrite(14,950); //가습기 틀기(모터돌리기)  
    
}
void setup() {
  
  Serial.begin(115200);
  pinMode(16, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(trigPin, OUTPUT);    //Trig 핀 output으로 세팅
  pinMode(echoPin, INPUT);    //Echo 핀 input으로 세팅
  //pinMode(5, OUTPUT);
//--------------------------------------Wifi connection--------------------------------------------------------------------------------  
  WiFi.mode(WIFI_AP_STA);
  Serial.printf("WiFimode:%d\r\n",WiFi.getMode());
  WiFi.begin("STRADALE_2G","jhome7791!");
  //WiFi.begin("HWAN","1234567890");
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
  //myHTTPServer.on("/DHT11",DHT11);
  myHTTPServer.begin(); //웹 서버 오픈

  calibrateSensor();
  for(int a=0; a<1000; a++)
  {
    Kalman();
    House_avg+=angleFiY;     
  }
  House_avg=House_avg/1000;
  past = millis();
  weather_time = millis();
}


void loop() {
  Kalman();
  Detection = abs(House_avg - angleFiY);// IFTTT 나 다른 함수가 들어가면 값 에러
  Serial.printf("angle; %f, avg: %f, Detection:%f \r\n",angleFiY, House_avg, Detection);
  
  //Serial.printf("House_avg: %f, angle; %f\r\n",House_avg,angleFiY);
  //Serial.printf("기울어진 값:%f\r\n",Detection);
  myHTTPServer.handleClient();
}
