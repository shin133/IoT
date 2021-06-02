#include <ESP8266WebServer.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define DELAY_MS 5500
#define DHT11PIN 12

int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

int MPU_Address=0x68;

int readTmp;
int readHumid;

char out_temp[10];
char in_temp[10];
char in_hum[10];

char AcX_B[10];
char AcY_B[10];
char AcZ_B[10];
char GyX_B[10];
char GyY_B[10];
char GyZ_B[10];
char Distance_b[10];

long duration, distance; 

int trigPin=13; //초음파 센서 trig
int echoPin=15;  //초음파 센서 echo


HTTPClient myclient;
DynamicJsonDocument doc(2048);
ESP8266WebServer myHTTPServer(80);//포트번호
unsigned long long lastMs=0;
unsigned long long loopMs=0;
//String str;

void ECHO(void){
    char ECHO_string[400];
    
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
      sprintf(Distance_b, "%d",distance);

      strcpy(ECHO_string, "<meta charset=utf-8>");
      strcat(ECHO_string, "센서에 포착된 사람과의 거리: ");
      strcat(ECHO_string, Distance_b);
      strcat(ECHO_string, " cm");
      strcat(ECHO_string, "<br>");
      myHTTPServer.send(200, "text/html", ECHO_string);
    }
}


void MPU6050(void){
    char MPU6050_string[400];
    if(millis()- lastMs >= DELAY_MS)
    {
    lastMs = millis(); //esp가 켜지고 몇초 지남?
    Wire.beginTransmission(MPU_Address);
    Wire.write(0x3B);
    Wire.endTransmission();  
    Wire.requestFrom(MPU_Address,14,true);  

    AcX=Wire.read() << 8 | Wire.read(); 
    AcY=Wire.read() << 8 | Wire.read();
    AcZ=Wire.read() << 8 | Wire.read();
    Tmp=Wire.read() << 8 | Wire.read();
    GyX=Wire.read() << 8 | Wire.read();
    GyY=Wire.read() << 8 | Wire.read();
    GyZ=Wire.read() << 8 | Wire.read();

    //readDHT11(&readTmp,&readHumid);
    
    Serial.printf("AcX= %d, AcY= %d, AcZ= %d, GyX= %d, GyY=%d, GyZ=%d\r\n", AcX, AcY, AcZ, GyX, GyY, GyZ); 
    
    sprintf(AcX_B, "%d",AcX); 
    sprintf(AcY_B, "%d",AcY); 
    sprintf(AcZ_B, "%d",AcZ); 
    sprintf(GyX_B, "%d",GyX); 
    sprintf(GyY_B, "%d",GyY); 
    sprintf(GyZ_B, "%d",GyZ); 

    strcpy(MPU6050_string, "<meta charset=utf-8>");
    strcat(MPU6050_string, "AcX: ");
    strcat(MPU6050_string, AcX_B);
    strcat(MPU6050_string, "<br>");

    strcat(MPU6050_string, "AcY: ");
    strcat(MPU6050_string, AcY_B);
    strcat(MPU6050_string, "<br>");
    
    strcat(MPU6050_string, "AcZ: ");
    strcat(MPU6050_string, AcZ_B);
    strcat(MPU6050_string, "<br><br>");
    
    strcat(MPU6050_string, "GyX: ");
    strcat(MPU6050_string, GyX_B);
    strcat(MPU6050_string, "<br>");
    
    strcat(MPU6050_string, "GyY: ");
    strcat(MPU6050_string, GyY_B);
    strcat(MPU6050_string, "<br>");
    
    strcat(MPU6050_string, "GyZ: ");
    strcat(MPU6050_string, GyZ_B);
    strcat(MPU6050_string, "<br><br>");

    strcat(MPU6050_string, "지진 발생 여부 : ");
    myHTTPServer.send(200, "text/html", MPU6050_string);
   
    //sprintf(temp_s, "%f",(Tmp/340.00+36.53));  
    }
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
      sprintf(out_temp, "%f",(float)tmp);
      strcat(weather_string, out_temp);
      myHTTPServer.send(200, "text/html", weather_string);
      
      if(whe=="Rain")
        digitalWrite(16, 0); //led로 비 경고
      else
        digitalWrite(16, 1);
    }
    else
    {
      Serial.printf("Server something wrong\r\n");  
    }
    
    

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
  //myHTTPServer.on("/DHT11",DHT11);
  
  myHTTPServer.begin(); //웹 서버 오픈
  delay(2000);
}


void loop() {
  myHTTPServer.handleClient();
}
