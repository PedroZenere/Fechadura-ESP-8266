#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <MFRC522.h>

#ifndef STASSID
#define STASSID "android_5G"
#define STAPSK  "%963741$"
#endif

#define RST_PIN    D3    
#define SS_PIN     D8   

const char* ssid = STASSID;
const char* password = STAPSK;

//DEFINIÇÃO DE IP FIXO PARA O NODEMCU
IPAddress ip(192,168,0,103); //COLOQUE UMA FAIXA DE IP DISPONÍVEL DO SEU ROTEADOR. EX: 192.168.1.110 **** ISSO VARIA, NO MEU CASO É: 192.168.0.175
IPAddress gateway(192,168,0,1); //GATEWAY DE CONEXÃO (ALTERE PARA O GATEWAY DO SEU ROTEADOR)
IPAddress subnet(255,255,255,0); //MASCARA DE REDE

ESP8266WebServer server(80);
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

const int led = 2;
const int pinRele = 4;
const int pinVerde = 16;
const int pinVermelho = 5;

String conteudo = "";
bool cadastrado = false;

String rfid[] = {"C6 35 A5 2B", "04 3A 8E D2 22 6D 80", "04 3E 8E D2 22 6D 80", "04 36 8E D2 22 6D 80", "04 2E 8E D2 22 6D 80", "04 2A 8E D2 22 6D 80", "04 26 8E D2 22 6D 80", "04 22 8E D2 22 6D 80", "04 1E 8E D2 22 6D 80", "04 1A 8E D2 22 6D 80", "04 16 8E D2 22 6D 80", "04 32 8E D2 22 6D 80", "04 12 8E D2 22 6D 80"};

void handleRoot() {
  digitalWrite(led, 1);
  char temp[1000];
  
  snprintf(temp, 1000,
    "<html>\
      <head>\
        <title>ESP32 Web Server</title>\
        <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\
        <meta name='viewport' content='width=device-width, initial-scale=1'>\
        <style>\
          html {  font-family: Arial, Helvetica, Sans-Serif; margin: 0px auto; text-align: center; }\
          body { background-color: #cccccc; }\
          header { display: flex; align-items: row;}\
          h1 { font-size: 20px; font-weight: bold; color: #0F3376; margin-left: 20px;}\
          button { display: inline-block; background-color: #008CBA; border: none; border-radius: 4px; color: white; text-decoration: none; font-size: 30px; cursor: pointer; padding: 16px 30px;}\
        </style>\
      </head>\
      <body>\
        <header>\
            <img src='https://i.ibb.co/SVvGxG7/logo-Politec-email.png' alt='logo' height='42' width='80'; margin-right: 8px; />\
            <h1>Controlador de Porta</h1>\
        </header>\
        <div>\
          <a href='/ABRIR'><button>ABRIR</button>\</a>\
        </div>\
      </body>\
  </html>");
  server.send(200, "text/html", temp);
  digitalWrite(led, 0);
}

void handleOpen() {
  digitalWrite(pinRele, 0);
  digitalWrite(pinVerde, 1);
  server.send(200, "/", "Porta aberta com sucesso");
  server.send(200, "/");
  delay(3000);
  digitalWrite(pinRele, 1);
  digitalWrite(pinVerde, 0);
  mfrc522.PCD_Init();
}

void abrirPorta() {
  digitalWrite(pinRele, 0); // ativa rele, abre a trava solenoide
  digitalWrite(pinVerde, 1);
  delay(3000);           // espera 3 segundos
  digitalWrite(pinRele, 1);// desativa rele, fecha a trava solenoide
  digitalWrite(pinVerde, 0);
}

void handleInitMFRC(){
  mfrc522.PCD_Init();
  conteudo = "";
  cadastrado = true;
}


void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void) {
  pinMode(led, OUTPUT);
  pinMode(pinRele, OUTPUT);
  pinMode(pinVerde, OUTPUT);
  pinMode(pinVermelho, OUTPUT);
  digitalWrite(led, 0);
  digitalWrite(pinVerde, 0);
  digitalWrite(pinVermelho, 0);
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet); //PASSA OS PARÂMETROS PARA A FUNÇÃO QUE VAI SETAR O IP FIXO NO NODEMCU

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/ABRIR", handleOpen);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
  digitalWrite(pinRele, 1);
}

void loop(void) {
  server.handleClient();
  MDNS.update();
  
   // Look for new cards
  if (mfrc522.PICC_IsNewCardPresent()){
    if (mfrc522.PICC_ReadCardSerial()) {
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      mfrc522.PICC_HaltA (); // Pare de ler
      conteudo.toUpperCase();
    }
    Serial.println();
    if(conteudo != ""){
      for(int i=0; i < sizeof(rfid); i++) {
        if(conteudo.substring(1) == rfid[i]){
          Serial.println("Encontrei !");
          Serial.println();
          abrirPorta();
          handleInitMFRC();
          break;
        }
      }
    }
    else if(conteudo != "" && cadastrado == false) {
      digitalWrite(pinVermelho, 1);
      delay(5000);
      digitalWrite(pinVermelho, 0);
      handleInitMFRC();
    }
  }
  cadastrado = false;
}
