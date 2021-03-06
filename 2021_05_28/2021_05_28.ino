#include <ESP8266WebServer.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define DELAY_MS 2000
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
int MPU_Address=0x68;

HTTPClient myclient;
DynamicJsonDocument doc(2048);
ESP8266WebServer myHTTPServer(80);//포트번호
unsigned long long lastMs=0;
//String str;

void fnRoot(void)
{
  char tmpBuffer[2000];
  strcpy(tmpBuffer, "<html>\r\n");
  strcat(tmpBuffer, "IOT Smart Home System Project<br><br>\r\n");
  strcat(tmpBuffer, "<a href=/status>Weather</a><br>\r\n");
  
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
      strcat(weather_string, "현재 날씨는: ");
      strcat(weather_string, whe);
      myHTTPServer.send(200, "text/html", weather_string);
      if(whe=="Rain")
      {
        digitalWrite(16, 0);
      }
    }
    else
    {
      Serial.printf("Server something wrong\r\n");  
    }
    
    

  }

void setup() {
  
  Serial.begin(115200);
  pinMode(16, OUTPUT);
  
  //pinMode(5, OUTPUT);
//--------------------------------------Wifi connection--------------------------------------------------------------------------------  
  WiFi.mode(WIFI_AP_STA);
  Serial.printf("WiFimode:%d\r\n",WiFi.getMode());
  //WiFi.begin("STRADALE_2G","jhome7791!");
  WiFi.begin("SK_WiFiGIGAEB90_2.4G","1903034280");
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
  myHTTPServer.begin(); //웹 서버 오픈
  delay(2000);
}


void loop() {
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

    Serial.printf("  = "); 
    //sprintf(temp_s, "%f",(Tmp/340.00+36.53));  
  }
  myHTTPServer.handleClient();
}
