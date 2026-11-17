/*
 * ESP32 + SSD1306 (128x64, I2C)
 * Relogio NTP + Clima (Open-Meteo) + Cotacao (AwesomeAPI)
 *
 * Bibliotecas (Gerenciador de Bibliotecas da Arduino IDE):
 *   - Adafruit GFX Library
 *   - Adafruit SSD1306
 *   - ArduinoJson (versao 7.x)
 *
 * Ligacao: SDA -> GPIO21 | SCL -> GPIO22 | VCC -> 3V3 | GND -> GND
 * Botao BOOT (GPIO0): pula para a proxima tela.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// ===================== CONFIGURACAO =====================
// Dados pessoais ficam em secrets.h (nao vai para o GitHub). Copie de secrets.example.h.
// Define: WIFI_SSID, WIFI_PASS, LATITUDE, LONGITUDE e CIDADE.
#include "secrets.h"

const char* TZ_INFO = "<-03>3";   // Brasilia (UTC-3, sem horario de verao)

const unsigned long TEMPO_TELA_MS   = 6000;       // quanto tempo cada tela fica
const unsigned long CLIMA_A_CADA_MS = 10UL * 60UL * 1000UL;  // 10 min
const unsigned long COTACAO_A_CADA_MS = 5UL * 60UL * 1000UL; // 5 min
const unsigned long RETRY_MS        = 30UL * 1000UL;         // tenta de novo apos falha

#define OLED_ADDR   0x3C
#define OLED_W      128
#define OLED_H      64
#define PIN_BOTAO   0
// ========================================================

Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

struct Clima {
  bool valid = false;
  float temp = 0, sens = 0, vento = 0, tmin = 0, tmax = 0;
  int umid = 0, codigo = 0, chuvaProb = 0;
} clima;

struct Cotacao {
  bool valid = false;
  float usd = 0, eur = 0, btc = 0;
  float usdP = 0, eurP = 0, btcP = 0;
} cotacao;

int telaAtual = 0;               // 0 relogio, 1 clima, 2 cotacao
const int NUM_TELAS = 3;
unsigned long ultimaTroca = 0;
unsigned long ultimoDesenho = 0;
unsigned long ultimoBotao = 0;
unsigned long proximoClima = 0;
unsigned long proximaCotacao = 0;
unsigned long ultimaTentativaWifi = 0;
bool ntpIniciado = false;

String ultimoErro = "";
String erroClima = "";
String erroCotacao = "";

const char* DIAS[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"};

// ---------- utilidades de desenho ----------
void textoCentralizado(const String& s, int y, int tam) {
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(tam);
  display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_W - w) / 2, y);
  display.print(s);
}

void cabecalho(const char* titulo) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(titulo);
  if (WiFi.status() != WL_CONNECTED) {
    display.setCursor(122, 0);
    display.print("!");
  }
  display.drawFastHLine(0, 10, OLED_W, SSD1306_WHITE);
}

void seta(int x, int y, bool sobe) {
  if (sobe) display.fillTriangle(x, y + 8, x + 8, y + 8, x + 4, y, SSD1306_WHITE);
  else      display.fillTriangle(x, y, x + 8, y, x + 4, y + 8, SSD1306_WHITE);
}

const char* descricaoClima(int c) {
  if (c == 0) return "Limpo";
  if (c <= 2) return "Sol e nuv";
  if (c == 3) return "Nublado";
  if (c == 45 || c == 48) return "Neblina";
  if (c >= 51 && c <= 57) return "Garoa";
  if (c >= 61 && c <= 67) return "Chuva";
  if (c >= 71 && c <= 77) return "Neve";
  if (c >= 80 && c <= 82) return "Pancadas";
  if (c >= 95) return "Tempestade";
  return "---";
}

// ---------- telas ----------
void telaRelogio() {
  struct tm t;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (!getLocalTime(&t, 0)) {
    cabecalho("RELOGIO");
    textoCentralizado("Sincronizando...", 28, 1);
    display.display();
    return;
  }

  char hm[6], ss[3], data[20];
  snprintf(hm, sizeof(hm), "%02d:%02d", t.tm_hour, t.tm_min);
  snprintf(ss, sizeof(ss), "%02d", t.tm_sec);
  snprintf(data, sizeof(data), "%s, %02d/%02d/%04d", DIAS[t.tm_wday], t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);

  display.setTextSize(3);
  display.setCursor(2, 4);
  display.print(hm);
  display.setTextSize(2);
  display.setCursor(98, 11);
  display.print(ss);

  textoCentralizado(data, 34, 1);

  display.drawFastHLine(0, 48, OLED_W, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 54);
  if (clima.valid) display.printf("%.0fC", clima.temp); else display.print("--C");
  display.setCursor(50, 54);
  if (cotacao.valid) display.printf("USD %.2f", cotacao.usd); else display.print("USD --");
  if (WiFi.status() != WL_CONNECTED) { display.setCursor(122, 54); display.print("!"); }

  display.display();
}

void telaClima() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  char titulo[32];
  snprintf(titulo, sizeof(titulo), "CLIMA - %s", CIDADE);
  cabecalho(titulo);

  if (!clima.valid) {
    textoCentralizado("Buscando dados...", 24, 1);
    if (erroClima.length()) textoCentralizado(erroClima, 38, 1);
    display.display();
    return;
  }

  // temperatura grande
  display.setTextSize(3);
  display.setCursor(0, 18);
  display.printf("%.0f", clima.temp);
  int x = display.getCursorX();
  display.drawCircle(x + 3, 20, 2, SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(x + 8, 18);
  display.print("C");

  // coluna da direita
  display.setTextSize(1);
  display.setCursor(68, 16); display.print(descricaoClima(clima.codigo));
  display.setCursor(68, 26); display.printf("Sens %.0fC", clima.sens);
  display.setCursor(68, 36); display.printf("Umid %d%%", clima.umid);
  display.setCursor(68, 46); display.printf("Vnt%.0fkm/h", clima.vento);

  // rodape
  display.setCursor(0, 56);
  display.printf("Min%.0f Max%.0f Chv%d%%", clima.tmin, clima.tmax, clima.chuvaProb);

  display.display();
}

void linhaCotacao(int y, const char* nome, float valor, float variacao, const char* fmt) {
  display.setTextSize(1);
  display.setCursor(0, y + 4);
  display.print(nome);
  display.setTextSize(2);
  display.setCursor(24, y);
  display.printf(fmt, valor);
  seta(118, y + 4, variacao >= 0);
}

void telaCotacao() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  cabecalho("COTACAO (R$)");

  if (!cotacao.valid) {
    textoCentralizado("Buscando dados...", 24, 1);
    if (erroCotacao.length()) textoCentralizado(erroCotacao, 38, 1);
    display.display();
    return;
  }

  linhaCotacao(15, "USD", cotacao.usd, cotacao.usdP, "%.2f");
  linhaCotacao(31, "EUR", cotacao.eur, cotacao.eurP, "%.2f");
  linhaCotacao(47, "BTC", cotacao.btc, cotacao.btcP, "%.0f");

  display.display();
}

// ---------- rede ----------
bool baixarJson(const String& url, JsonDocument& doc) {
  ultimoErro = "";
  if (WiFi.status() != WL_CONNECTED) { ultimoErro = "sem WiFi"; return false; }

  WiFiClientSecure client;
  client.setInsecure();   // sem validar certificado (simples; dados nao sao sensiveis)
  client.setTimeout(10);  // segundos
  HTTPClient http;
  http.setTimeout(10000);
  http.setConnectTimeout(8000);
  http.setReuse(false);
  if (!http.begin(client, url)) {
    ultimoErro = "begin falhou";
    Serial.println("[HTTP] begin falhou");
    return false;
  }
  int code = http.GET();
  // mostra so o servidor (a URL completa contem as coordenadas)
  Serial.printf("[HTTP] %s -> %d\n", url.substring(8, url.indexOf('/', 8)).c_str(), code);
  bool ok = false;
  if (code == 200) {
    String corpo = http.getString();
    DeserializationError err = deserializeJson(doc, corpo);
    if (err == DeserializationError::Ok) {
      ok = true;
    } else {
      ultimoErro = String("JSON ") + err.c_str();
      Serial.printf("[JSON] erro: %s\n", err.c_str());
    }
  } else if (code < 0) {
    ultimoErro = "conexao " + String(code);
    Serial.printf("[HTTP] erro: %s\n", http.errorToString(code).c_str());
  } else {
    ultimoErro = "HTTP " + String(code);
  }
  http.end();
  return ok;
}

bool atualizarClima() {
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(LATITUDE, 2) +
               "&longitude=" + String(LONGITUDE, 2) +   // 2 casas (~1 km): suficiente para o clima e nao revela o endereco
               "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m"
               "&daily=temperature_2m_max,temperature_2m_min,precipitation_probability_max"
               "&timezone=America%2FSao_Paulo&forecast_days=1";
  JsonDocument doc;
  if (!baixarJson(url, doc)) return false;

  JsonObject cur = doc["current"];
  if (cur.isNull()) return false;
  clima.temp      = cur["temperature_2m"] | 0.0f;
  clima.sens      = cur["apparent_temperature"] | 0.0f;
  clima.umid      = cur["relative_humidity_2m"] | 0;
  clima.codigo    = cur["weather_code"] | 0;
  clima.vento     = cur["wind_speed_10m"] | 0.0f;
  clima.tmax      = doc["daily"]["temperature_2m_max"][0] | 0.0f;
  clima.tmin      = doc["daily"]["temperature_2m_min"][0] | 0.0f;
  clima.chuvaProb = doc["daily"]["precipitation_probability_max"][0] | 0;
  clima.valid = true;
  return true;
}

bool atualizarCotacao() {
  JsonDocument doc;
  if (!baixarJson("https://economia.awesomeapi.com.br/last/USD-BRL,EUR-BRL,BTC-BRL", doc)) return false;
  if (doc["USDBRL"].isNull()) return false;

  cotacao.usd  = atof(doc["USDBRL"]["bid"] | "0");
  cotacao.eur  = atof(doc["EURBRL"]["bid"] | "0");
  cotacao.btc  = atof(doc["BTCBRL"]["bid"] | "0");
  cotacao.usdP = atof(doc["USDBRL"]["pctChange"] | "0");
  cotacao.eurP = atof(doc["EURBRL"]["pctChange"] | "0");
  cotacao.btcP = atof(doc["BTCBRL"]["pctChange"] | "0");
  cotacao.valid = true;
  return true;
}

void mensagemInicial(const char* l1, const char* l2 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  textoCentralizado(l1, 24, 1);
  if (l2[0]) textoCentralizado(l2, 38, 1);
  display.display();
}

void garantirWifi(unsigned long agora) {
  if (WiFi.status() == WL_CONNECTED) {
    if (!ntpIniciado) {
      configTzTime(TZ_INFO, "pool.ntp.org", "time.google.com", "a.st1.ntp.br");
      ntpIniciado = true;
    }
    return;
  }
  if (agora - ultimaTentativaWifi > 15000) {
    ultimaTentativaWifi = agora;
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOTAO, INPUT_PULLUP);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 nao encontrado! Confira ligacao e endereco (0x3C / 0x3D).");
    while (true) delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(200);

  // varredura: mostra no Serial quais redes 2.4GHz o ESP32 enxerga
  mensagemInicial("Procurando redes...");
  int n = WiFi.scanNetworks();
  bool achou = false;
  Serial.printf("[SCAN] %d redes encontradas:\n", n);
  for (int i = 0; i < n; i++) {
    bool igual = (WiFi.SSID(i) == String(WIFI_SSID));
    if (igual) achou = true;
    Serial.printf("  %s \"%s\"  sinal %d dBm  canal %d\n",
                  igual ? "->" : "  ", WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
  }
  Serial.printf("[SCAN] sua rede \"%s\" %s\n", WIFI_SSID, achou ? "FOI encontrada (entao o problema e a senha)" : "NAO apareceu (nome errado, 5GHz ou longe demais)");
  if (!achou) mensagemInicial("Rede nao encontrada", WIFI_SSID);
  else mensagemInicial("Conectando WiFi...", WIFI_SSID);
  delay(1500);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long ini = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - ini < 20000) delay(250);

  if (WiFi.status() == WL_CONNECTED) {
    configTzTime(TZ_INFO, "pool.ntp.org", "time.google.com", "a.st1.ntp.br");
    ntpIniciado = true;
    Serial.printf("[WIFI] conectado, IP %s\n", WiFi.localIP().toString().c_str());
    mensagemInicial("WiFi conectado!", WiFi.localIP().toString().c_str());
  } else {
    Serial.printf("[WIFI] falhou, status %d (rede 2.4GHz? senha certa?)\n", WiFi.status());
    mensagemInicial("Sem WiFi", "tentando em segundo plano");
  }
  delay(1200);
  ultimaTroca = millis();
}

void loop() {
  unsigned long agora = millis();

  garantirWifi(agora);

  // atualizacoes periodicas
  if (WiFi.status() == WL_CONNECTED) {
    if ((long)(agora - proximoClima) >= 0) {
      bool ok = atualizarClima();
      erroClima = ok ? "" : ultimoErro;
      Serial.printf("[CLIMA] %s %s\n", ok ? "ok" : "FALHOU", erroClima.c_str());
      proximoClima = millis() + (ok ? CLIMA_A_CADA_MS : RETRY_MS);
    }
    if ((long)(agora - proximaCotacao) >= 0) {
      bool ok = atualizarCotacao();
      erroCotacao = ok ? "" : ultimoErro;
      Serial.printf("[COTACAO] %s %s\n", ok ? "ok" : "FALHOU", erroCotacao.c_str());
      proximaCotacao = millis() + (ok ? COTACAO_A_CADA_MS : RETRY_MS);
    }
  }

  // botao BOOT: proxima tela
  if (digitalRead(PIN_BOTAO) == LOW && agora - ultimoBotao > 300) {
    ultimoBotao = agora;
    telaAtual = (telaAtual + 1) % NUM_TELAS;
    ultimaTroca = agora;
  }

  // rotacao automatica
  if (agora - ultimaTroca >= TEMPO_TELA_MS) {
    telaAtual = (telaAtual + 1) % NUM_TELAS;
    ultimaTroca = agora;
  }

  // redesenha a ~5x por segundo (segundos do relogio andam certinho)
  if (agora - ultimoDesenho >= 200) {
    ultimoDesenho = agora;
    switch (telaAtual) {
      case 0: telaRelogio();  break;
      case 1: telaClima();    break;
      case 2: telaCotacao();  break;
    }
  }
}
