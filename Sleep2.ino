#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include "display.h"
#include "esp_sleep.h" // Para deep sleep

#define MESH_PREFIX "meshcbm123"
#define MESH_PASSWORD "cbmforever"
#define MESH_PORT 5555

// Pines del LED RGB
#define RED_PIN 5
#define GREEN_PIN 18
#define BLUE_PIN 19

Scheduler userScheduler;
painlessMesh mesh;

// Variables de tiempo
unsigned long awakeStartTime = 0;   // Para rastrear cuándo se recibió el mensaje
unsigned long awakeDuration = 5000; // El receptor permanecerá despierto durante 5 segundos

// Prototipos
void controlRgbLed(int redState, int greenState, int blueState);
void enterDeepSleep();

// Callbacks
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Mensaje recibido de nodo %u: %s\n", from, msg.c_str());

  // Encender el LED azul cuando se recibe un mensaje
  controlRgbLed(LOW, LOW, HIGH);

  // Parsear el mensaje JSON
  JSONVar myObject = JSON.parse(msg.c_str());

  if (JSON.typeof(myObject) == "undefined")
  {
    Serial.println("Error al parsear el JSON");
    return;
  }

  // Verificar que los campos necesarios existen en el JSON
  if (myObject.hasOwnProperty("node") && myObject.hasOwnProperty("temp") && myObject.hasOwnProperty("hum") && myObject.hasOwnProperty("datetime"))
  {
    int node = myObject["node"];
    double temp = myObject["temp"];
    double hum = myObject["hum"];
    String datetime = String((const char *)myObject["datetime"]);

    // Mostrar los datos recibidos
    Serial.print("Nodo: ");
    Serial.println(node);
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.println(" °C");
    Serial.print("Humedad: ");
    Serial.print(hum);
    Serial.println(" %");
    Serial.print("Fecha y hora: ");
    Serial.println(datetime);

    infoReceived = "Nodo: " + String(node) + "\nTemp: " + String(temp) + " C\nHum: " + String(hum) + " %\n" + datetime;

    // Control del LED RGB basado en la temperatura
    if (temp > 30.0)
    {
      // Si la temperatura es mayor a 30°C, encender el LED rojo
      controlRgbLed(HIGH, LOW, LOW);
    }
    else
    {
      // Si la temperatura es inferior a 30°C, encender el LED verde
      controlRgbLed(LOW, HIGH, LOW);
    }
  }
  else
  {
    Serial.println("Datos incompletos en el JSON recibido.");
  }

  // Mantenerse despierto por 5 segundos antes de entrar en deep sleep
  awakeStartTime = millis(); // Registrar el tiempo cuando se recibió el mensaje
}

void controlRgbLed(int redState, int greenState, int blueState)
{
  digitalWrite(RED_PIN, redState);
  digitalWrite(GREEN_PIN, greenState);
  digitalWrite(BLUE_PIN, blueState);
  if (redState == HIGH)
  {
    Serial.println("temperatura alta");
    displayText(infoReceived + "\ntemperatura alta");
  }
  else if (greenState == HIGH)
  {
    Serial.println("temperatura normal");
    displayText(infoReceived + "\ntemperatura normal");
  }
  else if (blueState == HIGH)
  {
    Serial.println("mensaje recibido");
    displayText(infoReceived + "\nmensaje recibido");
  }
}

void enterDeepSleep()
{
  Serial.println("Entering deep sleep for 5 seconds...");
  esp_sleep_enable_timer_wakeup(5 * 1000000); // Dormir por 5 segundos
  esp_deep_sleep_start();
}

void newConnectionCallback(uint32_t nodeId)
{
  Serial.printf("Nueva conexión, nodoId = %u\n", nodeId);
}

void changedConnectionCallback()
{
  Serial.printf("Conexiones cambiadas\n");
}

void setup()
{
  Serial.begin(115200);

  // Configurar los pines del LED RGB como salida
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Apagar todos los LEDs al inicio
  controlRgbLed(LOW, LOW, LOW);

  // Configuración de la red Mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  startDisplay();

  Serial.println("Receptor iniciado y esperando mensajes...");
}

void loop()
{
  mesh.update();

  // Verificar si han pasado los 5 segundos desde que se recibió el mensaje
  if (awakeStartTime != 0 && millis() - awakeStartTime >= awakeDuration)
  {
    enterDeepSleep(); // Entrar en deep sleep después de estar despierto por 5 segundos
  }
}
