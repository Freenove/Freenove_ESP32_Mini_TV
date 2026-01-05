#include <WiFi.h>
#include <WebServer.h>
#include "web.h"
// ----------------------------------------------------------------
// WiFi configuration
// ----------------------------------------------------------------
const char* ssid =     "********";
const char* password = "********";

// Create a web server instance listening on HTTP port 80.
WebServer server(80);

// Serve an HTML page to the client.
void handleRoot() {
  server.send(200, "text/html", index_html);
}

// Decode the POST request data.
void handleSubmit() {
  if (server.hasArg("msg")) {
    String receivedMsg = server.arg("msg");
    Serial.println("-----------------------------------------------------------");
    Serial.print("Received message: ");
    Serial.println(receivedMsg);
    Serial.println("-----------------------------------------------------------");

    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

// Process 404 (Not Found) error.
void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.print("Connect to WiFi: ");
  Serial.println(ssid);
  
  // Set up the WiFi connection.
  WiFi.begin(ssid, password);

  // Wait until WiFi connects successfully
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print the assigned local IP address
  Serial.println("\nWiFi connected successfully!");
  Serial.print("Access address: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/submit", handleSubmit);
  server.onNotFound(handleNotFound);

  // Start the HTTP server
  server.begin();
  Serial.println("HTTP Server is running");
}

void loop() {
  // Continuously listen for and handle HTTP connection requests from clients
  server.handleClient();
}