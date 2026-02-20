#include <WiFi.h>               // Gestión de conexión Wi-Fi
#include <WiFiClientSecure.h>   // Conexión segura (HTTPS) necesaria para Spotify
#include <TFT_eSPI.h>           // Librería principal de la pantalla
#include <SpotifyArduino.h>     // Comunicación con la API de Spotify
#include <ArduinoJson.h>        // Procesamiento de datos JSON de Spotify
#include <HTTPClient.h>         // Cliente para descargar datos de la web (imagen)
#include <TJpg_Decoder.h>       // Decodificador de imágenes JPG
#include "time.h"               // Librería interna para manejo de tiempo (NTP)

// -- 1. CREDENCIALES --
char ssid[] = "TU_WIFI"; // Usuario
char password[] = "TU_PASSWORD"; // Contraseña
char clientId[] = "TU_CLIENTID"; // Id
char clientSecret[] = "TU_CLIENTSECRET"; // Secret
char refreshToken[] = "TU_REFRESHTOKEN"; // Refresh Token

// -- 2. OBJETOS --
WiFiClientSecure client;
SpotifyArduino spotify(client, clientId, clientSecret, refreshToken);
TFT_eSPI tft = TFT_eSPI();

// -- 3. VARIABLES GLOBALES Y ESTADO DEL SISTEMA --

// Configuración de Tiempo (NTP)
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000; // Ajustar segun zona horaria (GMT-5 para Perú)
const int   daylightOffset_sec = 0; // Horario de verano (0 seg para Perú)

// Estado de Conexión y Spotify
unsigned long ultimaVez = 0;        // Tiempo del último chequeo de API
String cancionActual = "";          // ID de la canción para detectar cambios
bool modoStandby = false;           // Estado de pantalla de espera (Logo/Reloj)

// Gestión de Pausa y Temporizador (2 minutos)
bool estaEnPausa = false;           // Estado de pausa en la reproducción
unsigned long tiempoInicioPausa = 0; // Marca de tiempo cuando se activó la pausa
const unsigned long TIEMPO_MAX_PAUSA = 120000; // Límite de 2 min antes de Standby

// Control de Interfaz y Scroll Dinámico
String tituloParaScroll = "";       // Título de la canción a desplazar
String artistaFijo = "";            // Nombre del artista
int offsetScroll = 0;               // Posición actual del texto
unsigned long ultimaVezScroll = 0;  // Tiempo de la última actualización del scroll
long progresoMs = 0;                // Progreso actual de la canción en milisegundos
long duracionMs = 0;                // Duración total de la canción en milisegundos
int estadoScroll = 0;               // 0: Quieto, 1: En movimiento
unsigned long cronometroPausa = 0;  // Tiempo de espera entre ciclos de scroll
bool esPausaInicial = true;         // Define si es el primer arranque de la canción

// -- 4. FUNCIÓN PUENTE (Callback) --
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) { // Pantalla recibe bloques de pixeles ya descomprimidos del tjpg decoder
  if ( y >= tft.height() ) return false; // Verifica si esta dentro de los limites (Seguridad)
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

// -- 5. FUNCIÓN PARA DESCARGAR LA IMAGEN --
void dibujarAlbum(const char* url) {
  HTTPClient http; 
  http.begin(url); // Inicia la conexion
  http.setTimeout(5000); // Tiempo para descargar
  int httpCode = http.GET(); // Respuesta del servidor
  if (httpCode == HTTP_CODE_OK) { 
    int len = http.getSize(); // Peso de la imagen 
    uint8_t* jpgBuffer = (uint8_t*)malloc(len); // Reserva un espacio en la RAM del ESP32
    if (jpgBuffer != NULL) { // Si paso correctamente
      WiFiClient* stream = http.getStreamPtr(); 
      int totalLeido = 0;
      unsigned long tiempoInicio = millis(); // Empieza a contar (Cronometro millis)
      while (totalLeido < len && (millis() - tiempoInicio < 5000)) { // Timeout de seguridad, le da 5 segundos sino continua
        if (stream->available()) {
          int leido = stream->read(jpgBuffer + totalLeido, len - totalLeido);
          totalLeido += leido;
        }
      }
      if (totalLeido >= len * 0.9) { // Minimo leido del 90%, se puede ajustar al 100% o menos
        TJpgDec.setJpgScale(4); // Escalado entre 4 (Ingresa en 300x300 -> Salen en 75x75)
        TJpgDec.drawJpg(26, 15, jpgBuffer, len); // Descomprime y pasa al puente
      }
      free(jpgBuffer); // Libera espacio de la RAM
    }
  }
  http.end(); // Cierra la conexion 
}

// -- 6. FUNCIÓN PARA EL MOVIMIENTO DEL TEXTO --
void manejarScroll() {
  if (modoStandby) return; // Ahorrar recursos si esta en Standby

  if (tituloParaScroll.length() > 16) { // Solo si el titulo es muy largo
    if (estadoScroll == 0) {
      if (cronometroPausa == 0) cronometroPausa = millis();
      unsigned long tiempoEspera = esPausaInicial ? 3000 : 10000; // La primera vez espera 3 segundos, las demas 10 antes de comenzar a moverse
      if (millis() - cronometroPausa > tiempoEspera) {
        estadoScroll = 1;
        cronometroPausa = 0;
        esPausaInicial = false;
      }
    } 
    else if (estadoScroll == 1) {
      if (millis() - ultimaVezScroll > 400) { // Cada 400ms desplazara los caracteres
        offsetScroll++; 
        tft.fillRect(0, 105, 128, 16, TFT_BLACK); // Limpieza selectiva para evitar parpadeos molestos
        tft.setTextColor(TFT_WHITE, TFT_BLACK); // Color texto - fondo
        tft.setCursor(5, 105); // Ubicacion del cursor

        if (offsetScroll > tituloParaScroll.length()) { // Para reinciar el Scroll
          offsetScroll = 0; 
          estadoScroll = 0;
          tft.setTextFont(2);
          tft.print(tituloParaScroll.substring(0, 16)); 
        } else { // Truco para Sroll infinito (barra transportadora)
          tft.setTextFont(2);
          String duplicado = tituloParaScroll + "    " + tituloParaScroll;
          tft.print(duplicado.substring(offsetScroll, offsetScroll + 16)); 
        }
        ultimaVezScroll = millis();
      }
    }
  }
}

// -- 7. FUNCIÓN PARA LA BARRA --
void dibujarBarra() {
  if (modoStandby) return; 

  int barraAncho = 100;
  int barraAlto = 8;
  int barraX = 14;
  int barraY = 145;

  tft.drawRect(barraX, barraY, barraAncho, barraAlto, TFT_WHITE); // Borde blanco para la barra
  if (duracionMs > 0) {
    float progreso = (float)progresoMs / duracionMs; // Normalizacion 0-1 
    int anchoActual = (int)(barraAncho * progreso); // Calculo escalado para % de barra
    
    uint16_t colorBarra = (tiempoInicioPausa > 0) ? 0x7BEF : TFT_GREEN; // Gris (Pausa) - Verde (Play), si contador > 0 comenzo la pausa

    if (anchoActual > 0) tft.fillRect(barraX + 1, barraY + 1, anchoActual, barraAlto - 2, colorBarra); // Pinta el progreso actual
    int anchoRestante = (barraAncho - 2) - anchoActual; 
    if (anchoRestante > 0) tft.fillRect(barraX + 1 + anchoActual, barraY + 1, anchoRestante, barraAlto - 2, TFT_BLACK); // Para evitar parpadeos, pinta el ancho restante 
  }
}

// -- 8. FUNCIÓN: DIBUJAR PANTALLA DE ESPERA --
void dibujarStandby() {
  tft.fillScreen(TFT_BLACK);
  int baseX = 35; 
  int baseY = 35;
  
  // Logo
  tft.fillRoundRect(baseX - 8, baseY + 6, 45, 18, 9, TFT_BLUE); 
  tft.fillRoundRect(baseX + 12, baseY + 14, 45, 18, 9, TFT_BLUE);
  tft.fillRoundRect(baseX, baseY, 45, 18, 9, TFT_LIGHTGREY);
  tft.fillRoundRect(baseX + 20, baseY + 8, 45, 18, 9, TFT_LIGHTGREY);

  struct tm timeinfo; // Descompone en hh mm ss 
  if (getLocalTime(&timeinfo)) { // Sincroniza hora actual mediante WiFI
    char horaActual[10];
    strftime(horaActual, 10, "%H:%M", &timeinfo); // Formato
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString(horaActual, 64, 85, 4); // Alineacion en pantalla
  }

  tft.setTextColor(0x7BEF, TFT_BLACK); 
  tft.setTextFont(2);
  tft.drawCentreString("Standby Mode", 64, 135, 2); // (String, x, y, tipografia con tamaños fijos)
}

// -- 9. FUNCIÓN DE DIBUJO DE INTERFAZ --
void dibujarInterfaz(CurrentlyPlaying data) {
  if (!data.isPlaying) { // Manejar pausa y temporizador
    if (tiempoInicioPausa == 0) tiempoInicioPausa = millis();
    
    if (millis() - tiempoInicioPausa > TIEMPO_MAX_PAUSA) {
      if (!modoStandby) { 
        modoStandby = true;
        cancionActual = ""; 
        dibujarStandby();
      }
      return;
    }
  } else { // Si hay música sonando
    tiempoInicioPausa = 0;
    if (modoStandby) {
      modoStandby = false; 
      tft.fillScreen(TFT_BLACK);
    }
  }

  if (modoStandby) return;

  // Dibujo normal de la interfaz
  if (String(data.trackUri) != cancionActual) { // Eficiencia de dibujo, detecta si cambio de cancion o no
    cancionActual = String(data.trackUri); // Obtiene id unnico de la cancion
    tituloParaScroll = String(data.trackName); // Obtiene titulo
    artistaFijo = String(data.artists[0].artistName); // Obtiene artista
    duracionMs = data.durationMs; // Obtiene duracion cancion

    offsetScroll = 0; estadoScroll = 0; cronometroPausa = 0; esPausaInicial = true; // Para el reseteo de variables
    
    // Album
    tft.fillScreen(TFT_BLACK); 
    dibujarAlbum(data.albumImages[1].url); 
    
    // Artista
    tft.setTextColor(TFT_GREEN, TFT_BLACK); 
    tft.setCursor(6, 125); tft.setTextFont(1);
    if (artistaFijo.length() > 20) tft.print(artistaFijo.substring(0, 20));  // Limitamos el nombre del artista para no sobrepasar pantalla
    else tft.print(artistaFijo + "          ");

    // Cancion
    tft.setTextColor(TFT_WHITE, TFT_BLACK); 
    tft.setCursor(6, 105); tft.setTextFont(2); 
    if (tituloParaScroll.length() > 16) tft.print(tituloParaScroll.substring(0, 16)); // Limitamos el nombre de la cancion para no sobrepasar pantalla
    else tft.print(tituloParaScroll + "    "); 
  }
  progresoMs = data.progressMs;
}

// -- 10. SETUP --
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(2); // Orientacion de la pantalla vertical inferior
  tft.setSwapBytes(true); // Correcion del color invertido 
  TJpgDec.setCallback(tft_output); 
  tft.fillScreen(TFT_BLACK); 
  
  WiFi.begin(ssid, password); // Intento de conexion WiFi
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); } 

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Conexion para pedir hora actual
  client.setInsecure(); // Para que funcione la API de Spotify (Conexion confiable)
  tft.fillScreen(TFT_BLACK); 
}

// -- 11. LOOP PRINCIPAL --
void loop() {
  if (millis() > ultimaVez + 1200) { // Consulta a la API cada 1.2 segundos
    int code = spotify.getCurrentlyPlaying(dibujarInterfaz, "ES");
    
    if (code != 200 && !modoStandby) { // Si la App está cerrada o no hay dispositivo activo
      modoStandby = true;
      cancionActual = "";
      dibujarStandby();
    }
    ultimaVez = millis(); 
  }

  if (!modoStandby) { // Se encuentra en modo Standby
    manejarScroll();
    dibujarBarra();
  } else { // Se encuentra activo
    static int ultimoMinuto = -1;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) { 
      if (timeinfo.tm_min != ultimoMinuto) { // Para ahorrar recursos, solo actualizara cada minuto asi desconecte API o falle conexion 
        ultimoMinuto = timeinfo.tm_min;
        dibujarStandby(); // Refresca pantalla de standby (logo + hora)
      }
    }
  }
}
