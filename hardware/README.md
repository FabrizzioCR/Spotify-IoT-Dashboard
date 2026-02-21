# 🔌 Documentación de Hardware - OFF-SET v1.0

Este documento detalla la arquitectura física y el esquema de conexiones del Dashboard.

## 🖼️ Esquema Técnico
![Esquema de Conexiones](./schematic.png)

## 📌 Pinout (Mapeo de Etiquetas)

| Etiqueta Pantalla | Pin ESP32 | Función | Notas |
| :--- | :--- | :--- | :--- |
| **VDD** | 3.3V | VCC | Alimentación lógica |
| **GND** | GND | Ground | Tierra común |
| **SCL** | GPIO 18 | SCK (SPI) | Reloj del bus SPI |
| **SDA** | GPIO 23 | MOSI (SPI) | Transmisión de datos |
| **CS** | GPIO 5 | Chip Select | Selección de periférico |
| **DC** | GPIO 2 | Data/Command | Control de registro |
| **RST** | GPIO 4 | Reset | Reinicio de pantalla |
| **BLK** | 3.3V | Backlight | Iluminación (Fija en v1.0) |

### ⚠️ Notas de Ingeniería
* **Protocolo**: Se utiliza SPI unidireccional. El pin **MISO** se mantiene como **NC** (No Conectado).
* **Confusión I2C**: Aunque las etiquetas digan SDA/SCL, la pantalla opera bajo SPI. No conectar a los pines I2C estándar del ESP32.

## 🚀 Próximas Mejoras (v2.0)
* **Control de Brillo**: Implementación de PWM en el pin **BLK** (posiblemente GPIO 21) para sincronizar con el modo Standby.
* **Interfaz Física**: Adición de 3 botones tipo push-button para control de música:
    * **Next**: GPIO 13
    * **Play/Pause**: GPIO 12
    * **Previous**: GPIO 14

---
**Desarrollado por Fabrizzio como parte del portafolio de mecatrónica OFF-SET.**
*Estudiante de Ingeniería Mecatrónica en la Pontificia Universidad Católica del Perú (PUCP).*
