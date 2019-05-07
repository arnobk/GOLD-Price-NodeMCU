/*
  Name: Gold & Silver wap.kitco.com price display
  Author: Arnob Karmokar
  Version: 1.0
  Date Created: 07 March 2019
  Date Modified: 07 March 2019
*/


#include "Arduino.h"
#include <ESP8266WiFi.h>

WiFiClient client;

#define NUM_MAX 4
#define ROTATE 90

// for NodeMCU 1.0
#define DIN_PIN D7  // D7
#define CS_PIN  D4  // D4
#define CLK_PIN D5  // D5

#include "max7219.h"
#include "fonts.h"

// =======================================================================
// Your config below!
// =======================================================================
const char* ssid     = "AUST_SPRING16";      // SSID of local network
const char* password = "studio80";    // Password on network
const char* uniqueId = "UCV9vwhQm3DNYW-5NjhMd3yA";   // Your Unique id
// =======================================================================

void setup() 
{
  Serial.begin(115200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN,1);
  sendCmdAll(CMD_INTENSITY,0);
  Serial.print("Connecting with WiFi ");
  WiFi.begin(ssid, password);
  printStringWithShift("Connecting with WiFi ...",20);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);
  }
  Serial.println("");
  Serial.print("Connected: "); 
  Serial.println(WiFi.localIP());
}
// =======================================================================

void loop()
{
  Serial.println("Getting data ...");
  printStringWithShift("  Made By Arnob ",20);
  float goldPrice, silverPrice, cnt = 0;
  String price1,price2;
  while(1) {
    if(!cnt--) {
      cnt = 5;  // data is refreshed every 5 loops
      if(getPrice(uniqueId,&goldPrice,&silverPrice)==0) {
        price1 = "      GOLD:     "+String(goldPrice/100);
        price2 = "      SILVER:    "+String(silverPrice/100);
      } else {
        price1 = "   Ops!";
        price2 = "   Error!";
      }
    }
    printStringWithShift(price1.c_str(),20);
    delay(3000);
    printStringWithShift(price2.c_str(),20);
    delay(3000);
  }
}
// =======================================================================

int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i,w = pgm_read_byte(data + 1 + ch * len);
  for (i = 0; i < w; i++)
    scr[NUM_MAX*8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  scr[NUM_MAX*8 + i] = 0;
  return w;
}

// =======================================================================
int dualChar = 0;

unsigned char convertPolish(unsigned char _c)
{
  unsigned char c = _c;
  if(c==196 || c==197 || c==195) {
    dualChar = c;
    return 0;
  }
  if(dualChar) {
    switch(_c) {
      case 133: c = 1+'~'; break; // 'ą'
      case 135: c = 2+'~'; break; // 'ć'
      case 153: c = 3+'~'; break; // 'ę'
      case 130: c = 4+'~'; break; // 'ł'
      case 132: c = dualChar==197 ? 5+'~' : 10+'~'; break; // 'ń' and 'Ą'
      case 179: c = 6+'~'; break; // 'ó'
      case 155: c = 7+'~'; break; // 'ś'
      case 186: c = 8+'~'; break; // 'ź'
      case 188: c = 9+'~'; break; // 'ż'
      //case 132: c = 10+'~'; break; // 'Ą'
      case 134: c = 11+'~'; break; // 'Ć'
      case 152: c = 12+'~'; break; // 'Ę'
      case 129: c = 13+'~'; break; // 'Ł'
      case 131: c = 14+'~'; break; // 'Ń'
      case 147: c = 15+'~'; break; // 'Ó'
      case 154: c = 16+'~'; break; // 'Ś'
      case 185: c = 17+'~'; break; // 'Ź'
      case 187: c = 18+'~'; break; // 'Ż'
      default:  break;
    }
    dualChar = 0;
    return c;
  }    
  switch(_c) {
    case 185: c = 1+'~'; break;
    case 230: c = 2+'~'; break;
    case 234: c = 3+'~'; break;
    case 179: c = 4+'~'; break;
    case 241: c = 5+'~'; break;
    case 243: c = 6+'~'; break;
    case 156: c = 7+'~'; break;
    case 159: c = 8+'~'; break;
    case 191: c = 9+'~'; break;
    case 165: c = 10+'~'; break;
    case 198: c = 11+'~'; break;
    case 202: c = 12+'~'; break;
    case 163: c = 13+'~'; break;
    case 209: c = 14+'~'; break;
    case 211: c = 15+'~'; break;
    case 140: c = 16+'~'; break;
    case 143: c = 17+'~'; break;
    case 175: c = 18+'~'; break;
    default:  break;
  }
  return c;
}

// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay) {
  c = convertPolish(c);
  if (c < ' ' || c > MAX_CHAR) return;
  c -= 32;
  int w = showChar(c, font);
  for (int i=0; i<w+1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================

void printStringWithShift(const char* s, int shiftDelay){
  while (*s) {
    printCharWithShift(*s++, shiftDelay);
  }
}

// =======================================================================
unsigned int convToInt(const char *txt)
{
  unsigned int val = 0;
  for(int i=0; i<strlen(txt); i++)
    if(isdigit(txt[i])) val=val*10+(txt[i]&0xf);
  return val;
}
// =======================================================================

const char* apiHost = "api.thingspeak.com";
String url = "/apps/thinghttp/send_request?api_key=9AP6HITRX5ZX7WL8";  
int getPrice(const char *uniqueId, float *pGold, float *pSilver)
{
  if(!pGold || !pSilver) return -2;
  WiFiClient client;
  Serial.print("connecting to "); Serial.println(apiHost);
  if (!client.connect(apiHost, 80)) {
    Serial.println("connection failed");
    return -1;
  }
  client.print(String("GET ") + url + " HTTP/1.0\r\n" +
               "Host: " + apiHost + "\r\n" + 
               "Connection: close\r\n\r\n");
  int repeatCounter = 20;
  while (!client.available() && repeatCounter--) {
    Serial.println("y."); delay(500);
  }
  int idxS1, idxE1,idxS2,idxE2, statsFound = 0;
  *pGold = *pSilver = 0;
  while (client.available()) {
    String line = client.readStringUntil('\n');
    String val1,val2;
    Serial.println(line);
    
      idxS1 = line.indexOf("Gold");
      idxS2 = line.indexOf("Silver");
      //Serial.println(idxS);
      idxE1 = idxS1+12;
      idxE2 = idxS2+12;
      if(idxS1>=0){
        val1 = line.substring(idxS1+5, idxE1);
        Serial.println(val1);
        if(!*pGold)
        *pGold = convToInt(val1.c_str());
      } 
      if(idxS2>=0){
        val2 = line.substring(idxS2+7, idxE2);
        Serial.println(val2);
        *pSilver = convToInt(val2.c_str());
        break;
      }
      
      
  }
  client.stop();
  return 0;
}
