# 🎵 Spotify IoT Dashboard (ESP32) | OFF-SET Edition

![ESP32](https://img.shields.io/badge/Platform-ESP32-blue?logo=espressif)
![Status](https://img.shields.io/badge/Status-v1.0_Stable-green)
![3D Printing](https://img.shields.io/badge/3D_Printing-Bambu_Lab_P1S-008554?logo=bambulab)

Este proyecto es un dashboard inteligente diseñado para el ecosistema IoT que integra la visualización de música en tiempo real con una estación de tiempo funcional. Bajo la identidad visual de **OFF-SET**, el dispositivo gestiona estados de energía y visualización mediante una máquina de estados optimizada.

---

## 🚀 Características Técnicas (v1.0)

* **Gestión Inteligente de Estados (HMI)**: Activación automática de modo **Standby** tras 2 minutos de inactividad para proteger el panel TFT.
* **Reloj Global NTP**: Sincronización mediante Network Time Protocol para mostrar la hora exacta (GMT-5).
* **Scroll Dinámico No Bloqueante**: Algoritmo basado en `millis()` para manejar títulos largos sin detener otros procesos.
* **Procesamiento de Imágenes**: Decodificación de portadas JPG desde la API de Spotify en tiempo real.

## 🛠️ Hardware & Conexiones

| Componente | Especificación |
| :--- | :--- |
| **Microcontrolador** | ESP32 DevKit V1 |
| **Pantalla (Display)** | **TFT ST7735 LCD 1.8" (128x160 px)** |
| **Fabricación** | Carcasa diseñada y optimizada para **Bambu Lab P1S** |

### Diagrama de Pines (Pinout)

| TFT Pin | ESP32 Pin | Función (SPI) |
| :--- | :--- | :--- |
| **VCC** | 3.3V | Alimentación |
| **GND** | GND | Tierra |
| **SCL/SCK** | GPIO 18 | Clock |
| **SDA/MOSI**| GPIO 23 | MOSI |
| **RES** | GPIO 4 | Reset |
| **DC** | GPIO 2 | Data/Command |

> **Nota de compatibilidad:** Aunque el código es compatible con otras pantallas mediante la librería `TFT_eSPI`, este proyecto y su carcasa mecánica están optimizados para el modelo **ST7735**. Si utilizas un panel distinto, deberás ajustar la configuración de software y el diseño de la carcasa.

## 📂 Librerías y Dependencias

Para compilar este proyecto, instala las siguientes librerías:

### 📥 Instalación Manual (GitHub)
* **[SpotifyArduino](https://github.com/witnessmenow/spotify-api-arduino)**: Gestión de la API de Spotify. Descargar repositorio como .ZIP e instalar mediante el IDE.

### ⚙️ Instalación desde Library Manager (Arduino IDE)
1. **TFT_eSPI** (por Bodmer): Requiere configurar los archivos de la librería según el controlador de pantalla (ST7735).
2. **ArduinoJson** (por Benoit Blanchon): v6.x o superior.
3. **TJpg_Decoder** (por Bodmer): Para la decodificación de portadas.

## ⚙️ Configuración y Uso

### 1. Spotify Developer Dashboard
1. Crea una App en el [Spotify Developer Dashboard](https://developer.spotify.com/dashboard).
2. Configura la **Redirect URI** como: `https://developers.google.com/oauthplayground/callback/`.
3. Obtén tu `Client ID` y `Client Secret`.

### 2. Obtención del Refresh Token (OAuth Playground)
1. Ve a [Google OAuth Playground](https://developers.google.com/oauthplayground/).
2. En **Settings** (engranaje), marca **"Use your own OAuth credentials"** e ingresa tu Client ID y Secret.
3. En el Paso 1, ingresa manualmente: `user-read-playback-state` y `user-read-currently-playing`.
4. Autoriza y genera el **Refresh Token** permanente.

### 3. Configuración del Firmware
1. Configura `User_Setup.h` y `User_Setup_Select.h` en la carpeta de la librería **TFT_eSPI** para el controlador **ST7735**.
2. Ingresa tus credenciales Wi-Fi y tokens en el código fuente dentro de la carpeta `/firmware`.

---
**Desarrollado por Fabrizzio como parte del portafolio de mecatrónica OFF-SET.**
*Estudiante de Ingeniería Mecatrónica en la Pontificia Universidad Católica del Perú (PUCP).*
