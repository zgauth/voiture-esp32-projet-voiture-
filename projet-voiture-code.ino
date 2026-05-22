#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// === WiFi ===
const char* ssid = "cielatd1";
const char* password = "atdd1ciel";

// === Moteurs L293D ===
const int motorA1 = 5;
const int motorA2 = 4;
const int motorB1 = 0;
const int motorB2 = 2;

// === Servo ===
Servo directionServo;
const int servoPin = 15;
const int angleCentre = 90;
const int angleGauche = 45;
const int angleDroite = 135;

// === PWM vitesse ===
int vitesse = 255;  // 0-255

// === Serveur web ===
ESP8266WebServer server(80);

// === Fonctions moteurs ===
void avancer() {
  analogWrite(motorA1, vitesse);
  digitalWrite(motorA2, LOW);
  analogWrite(motorB1, vitesse);
  digitalWrite(motorB2, LOW);
  Serial.println("Avance");
}

void reculer() {
  digitalWrite(motorA1, LOW);
  analogWrite(motorA2, vitesse);
  digitalWrite(motorB1, LOW);
  analogWrite(motorB2, vitesse);
  Serial.println("Recule");
}

void arreter() {
  digitalWrite(motorA1, LOW);
  digitalWrite(motorA2, LOW);
  digitalWrite(motorB1, LOW);
  digitalWrite(motorB2, LOW);
  directionServo.write(angleCentre);
  Serial.println("Arrêt");
}

void tournerGauche() {
  directionServo.write(angleGauche);
  Serial.println("Gauche");
}

void tournerDroite() {
  directionServo.write(angleDroite);
  Serial.println("Droite");
}

// === Page web moderne ===
void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial; text-align: center; background: #222; color: #fff; margin:0; padding:0; }
      h1 { margin-top: 20px; }
      .button { width: 80px; height: 80px; font-size: 24px; margin: 10px; border-radius: 50%; border:none; color: #fff; }
      .forward { background: #4CAF50; }
      .back { background: #f44336; }
      .left { background: #2196F3; }
      .right { background: #FF9800; }
      .stop { background: #555; }
      #joystick { width: 200px; height: 200px; background: #555; border-radius: 50%; margin: 20px auto; position: relative; touch-action: none; }
      #stick { width: 80px; height: 80px; background: #999; border-radius: 50%; position: absolute; top: 60px; left: 60px; }
      #speedSlider { width: 80%; margin: 20px auto; }
      #status { margin-top: 10px; font-size: 18px; color: #0f0; }
    </style>
  </head>
  <body>
    <h1> Voiture Connecter</h1>

    <h2>Boutons</h2>
    <div>
      <button class="button forward" onclick="send('avancer')">↑</button><br>
      <button class="button left" onclick="send('gauche')">←</button>
      <button class="button stop" onclick="send('stop')">■</button>
      <button class="button right" onclick="send('droite')">→</button><br>
      <button class="button back" onclick="send('reculer')">↓</button>
    </div>

    <h2>Joystick</h2>
    <div id="joystick">
      <div id="stick"></div>
    </div>

    <h2>Vitesse</h2>
    <input type="range" id="speedSlider" min="0" max="255" value="255" oninput="changeSpeed(this.value)">
    <div id="status">Vitesse: 255</div>

    <script>
      function send(cmd) {
        fetch('/' + cmd);
      }

      function changeSpeed(val) {
        fetch('/speed?val=' + val);
        document.getElementById('status').innerHTML = 'Vitesse: ' + val;
      }

      const joystick = document.getElementById("joystick");
      const stick = document.getElementById("stick");
      const centerX = joystick.offsetWidth/2;
      const centerY = joystick.offsetHeight/2;
      let active = false;

      joystick.addEventListener("touchstart", e=>{active=true;}, false);
      joystick.addEventListener("touchend", e=>{active=false; stick.style.left='60px'; stick.style.top='60px'; send('stop');}, false);

      joystick.addEventListener("touchmove", e=>{
        if(!active) return;
        e.preventDefault();
        let touch = e.touches[0];
        let rect = joystick.getBoundingClientRect();
        let x = touch.clientX - rect.left;
        let y = touch.clientY - rect.top;
        let dx = x-centerX;
        let dy = y-centerY;
        let dist = Math.sqrt(dx*dx + dy*dy);
        let angle = Math.atan2(dy, dx);
        let maxDist = 60;
        if(dist>maxDist) dist=maxDist;
        let stickX = centerX + dist*Math.cos(angle)-stick.offsetWidth/2;
        let stickY = centerY + dist*Math.sin(angle)-stick.offsetHeight/2;
        stick.style.left = stickX+'px';
        stick.style.top = stickY+'px';

        if(dy<-30 && Math.abs(dx)<40) send('avancer');
        else if(dy>30 && Math.abs(dx)<40) send('reculer');
        else if(dx<-30 && Math.abs(dy)<40) send('gauche');
        else if(dx>30 && Math.abs(dy)<40) send('droite');
      }, false);
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html; charset=UTF-8", page);
}

// Routes
void setupRoutes() {
  server.on("/", handleRoot);
  server.on("/avancer", []() {
    avancer();
    server.send(200, "text/plain", "Avance");
  });
  server.on("/reculer", []() {
    reculer();
    server.send(200, "text/plain", "Recule");
  });
  server.on("/gauche", []() {
    tournerGauche();
    server.send(200, "text/plain", "Gauche");
  });
  server.on("/droite", []() {
    tournerDroite();
    server.send(200, "text/plain", "Droite");
  });
  server.on("/stop", []() {
    arreter();
    server.send(200, "text/plain", "Stop");
  });
  server.on("/speed", []() {
    if (server.hasArg("val")) {
      vitesse = server.arg("val").toInt();
      server.send(200, "text/plain", "OK");
    } else server.send(400, "text/plain", "Erreur");
  });
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connexion WiFi");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnecté ! IP:");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nÉchec WiFi");
    WiFi.softAP("Voiture_ESP8266", "12345678");
    Serial.println("IP AP: 192.168.4.1");
  }

  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);

  directionServo.attach(servoPin);
  directionServo.write(angleCentre);

  setupRoutes();
  server.begin();
  Serial.println("Serveur web démarré 🚀");
}

void loop() {
  server.handleClient();
}
