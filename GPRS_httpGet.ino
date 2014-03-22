#include "SIM900.h"
#include <SoftwareSerial.h>
#include <inetGSM.h>

InetGSM inet;

#define GSM_POWER_PIN 6

char msg[1];
boolean started = false;

int  bus_id    = 4;
long latitude  = -2142091;
long longitude = -5374203;

char charBuf[60];

void(* reset)(void) = 0;

void setup()
{
  pinMode(GSM_POWER_PIN, INPUT);
  
  Serial.begin(9600);
  Serial.println("Iniciando Shield GSM...");
  
  // clear old buffer
  while(Serial.available()) Serial.read();
  delay(1);
  
  startGSM();
}

void powerOnGSM()
{
  Serial.println("GSM Power ON");
  digitalWrite(GSM_POWER_PIN, HIGH);
  delay(3000);
}

void startGSM() 
{
  powerOnGSM();
  
  //Start configuration of shield with baudrate.
  //For http uses is raccomanded to use 4800 or slower.
  if (gsm.begin(9600))
  {
    Serial.println("\nstatus=GSM PRONTO");          
    started = true;
  } else Serial.println("\nstatus=GSM OCUPADO");

  if(started)
  {
    attachGPRS();
  }
}

void attachGPRS()
{
  while(!inet.attachGPRS("zap.vivo.com.br", "vivo", "vivo"))
  {
    Serial.println("attachGPRS(): 0 = unable to establish a GPRS connection");
    delay(1000);
  }   

  Serial.println("attachGPRS(): 1 = connection successfully established");
  delay(1000);

  //Read IP address.
  gsm.SimpleWriteln("AT+CIFSR");
  delay(5000);
  //Read until serial buffer is empty.
  gsm.WhileSimpleRead();
}

void loop()
{
  while(!inet.connectedClient())
  {
    Serial.println("Retry GPRS!");
    attachGPRS();
  }
  
  postData();
  gsmRead();
  getGPS();
}

void gsmRead()
{
  if(started)
  {
    Serial.print("Reading GSM...");
    gsm.WhileSimpleRead();
    Serial.println("Done!");
  }
}

void postData()
{
  int  result;
  char data[40] = "bus_id=";
  char c_bus[2];
  char c_lat[10];
  char c_lon[10];
  
  itoa(bus_id, c_bus, 10);
  ltoa(latitude, c_lat, 10);
  ltoa(longitude, c_lon, 10);

  strcat(data, c_bus);
  strcat(data, "&lat=");
  strcat(data, c_lat);  
  strcat(data, "&lon=");
  strcat(data, c_lon);

  httpPOST("www.ondebus.com.br", 80, "/position", data);
  
  Serial.print("Posting data: ");
  Serial.println(data);
  Serial.println("Done!");
}

void getGPS()
{
  latitude  += 5;
  longitude += 5;
    
  Serial.print("Updating position...    lat=");
  Serial.print(latitude);
  Serial.print("&lon=");
  Serial.print(longitude);
  Serial.println(" Done!");
}

int httpPOST(const char* server, int port, const char* path, const char* parameters)
{
  boolean connected=false;
  int n_of_at=0;
  char itoaBuffer[8];
  int num_char;
  char end_c[2];
  end_c[0]=0x1a;
  end_c[1]='\0';

  while(n_of_at<3) {
    if(!inet.connectTCP(server, port))
    {
      Serial.println("DB:NOT CONN");
      n_of_at++;
    } else {
      connected=true;
      n_of_at=3;
    }
  }

  if(!connected) return 0;

  gsm.SimpleWrite("POST ");
  gsm.SimpleWrite(path);
  gsm.SimpleWrite(" HTTP/1.1\r\nHost: ");
  gsm.SimpleWrite(server);
  gsm.SimpleWrite("\r\n");
  gsm.SimpleWrite("User-Agent: Arduino\r\n");
  gsm.SimpleWrite("Content-Type: application/x-www-form-urlencoded\r\n");
  gsm.SimpleWrite("Content-Length: ");
  itoa(strlen(parameters),itoaBuffer,10);
  gsm.SimpleWrite(itoaBuffer);
  gsm.SimpleWrite("\r\n\r\n");
  gsm.SimpleWrite(parameters);
  gsm.SimpleWrite("\r\n\r\n");
  gsm.SimpleWrite(end_c);
  
  switch(gsm.WaitResp(10000, 10, "SEND OK")) {
  case RX_TMOUT_ERR:
    return 0;
    break;
  case RX_FINISHED_STR_NOT_RECV:
    return 0;
    break;
   }
  
   delay(50);
   Serial.println("DB:SENT");
   
   inet.disconnectTCP();
   
   return 1;
}
