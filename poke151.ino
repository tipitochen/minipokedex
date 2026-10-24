#include <SPI.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "pokemon_data.h"//151

TFT_eSPI tft = TFT_eSPI();
Preferences preferences; 

// Pines de los botones
#define BTN_PREV 0   // Botón 1 (GPIO 0) -> SALTAR
#define BTN_NEXT 35  // Botón 2 (GPIO 35) -> AURA TRUENO

int pokemonActual = 0; 

// Variables para control de velocidad al mantener en modo manual
unsigned long pressStartTime = 0;
unsigned long lastStepTime = 0;
bool btnNextWasPressed = false;
bool btnPrevWasPressed = false;

// Variables del Menú y Modos
bool enMenu = false;
int modoActual = 0;      
int randomVelocidad = 5; 
int opcionMenu = 0;      
unsigned long ultimoCambioRandom = 0;

// Variables específicas para el Minijuego "Who's That Pokémon?"
bool minijuegoRevelado = false;
unsigned long tiempoFaseMinijuego = 0;

// --- VARIABLES DEL MINIJUEGO RUNNER ---
int pikaY = 100;          
int pikaVy = 0;           
bool enSuelo = true;
float pokebolaX = 240;    
int puntajeRunner = 0;
int highscoreRunner = 0;  
float velocidadRunner = 3.5;
unsigned long ultimoFrameRunner = 0;
bool juegoTerminado = false;
bool gameOverDibujado = false; // Control para que el Game Over sea estático

// Variables del Aura Trueno y Recarga
int truenosDisponibles = 2;
bool mostrandoAura = false;
unsigned long tiempoAura = 0;

// Variables para evitar parpadeo en la barra superior
int ultimoPuntajeDibujado = -1;
int ultimosTruenosDibujados = -1;
int ultimoHighscoreDibujado = -1;

// Prototipos
void mostrarPantallaBoot();
void mostrarPokemon(int index, bool silueta);
void dibujarMenu();
void ejecutarMinijuegoRunner(bool btnPrev, bool btnNext);

void mostrarPantallaBoot() {
  tft.setRotation(0);
  tft.fillScreen(0x18EF);
  tft.drawRect(5, 5, tft.width() - 10, tft.height() - 10, TFT_RED);
  tft.drawRect(7, 7, tft.width() - 14, tft.height() - 14, TFT_YELLOW);

  tft.setTextDatum(TC_DATUM);
  int centerX = tft.width() / 2;

  tft.setTextColor(TFT_CYAN, 0x18EF);
  tft.setTextSize(2);
  tft.drawString("ESP32", centerX, 45);

  tft.setTextColor(TFT_WHITE, 0x18EF);
  tft.setTextSize(2);
  tft.drawString("MINI", centerX, 70);

  tft.setTextColor(TFT_YELLOW, 0x18EF);
  tft.setTextSize(3);
  tft.drawString("POKEDEX", centerX, 100);

  tft.drawFastHLine(25, 140, tft.width() - 50, TFT_DARKGREY);

  int pbY = 175;
  int r = 22;
  for (int y = -r; y <= r; y++) {
    int w = sqrt(r * r - y * y);
    uint16_t color;
    if (y < -2) color = TFT_RED;
    else if (y >= -2 && y <= 2) color = TFT_BLACK;
    else color = TFT_WHITE;
    tft.drawFastHLine(centerX - w, pbY + y, 2 * w + 1, color);
  }
  tft.drawCircle(centerX, pbY, r, TFT_BLACK);

  tft.fillCircle(centerX, pbY, 7, TFT_WHITE);
  tft.drawCircle(centerX, pbY, 7, TFT_BLACK);
  tft.fillCircle(centerX, pbY, 3, TFT_DARKGREY);

  tft.setTextColor(TFT_LIGHTGREY, 0x18EF);
  tft.setTextSize(1);
  tft.drawString("Kanto Region - Gen 1", centerX, 215);

  delay(2500);
}

void mostrarPokemon(int index, bool silueta) {
  tft.setRotation(0); 

  if (silueta) {
    tft.fillScreen(0xF800); 

    int centerX = tft.width() / 2;
    int centerY = 100;
    tft.fillCircle(centerX, centerY, 62, TFT_WHITE);
    tft.drawCircle(centerX, centerY, 62, 0x07FF); 

    tft.fillRect(0, 0, tft.width(), 32, TFT_BLACK);
    tft.drawFastHLine(0, 32, tft.width(), TFT_YELLOW);

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("\xBFQuien es este Pokemon?", tft.width() / 2, 10);

  } else {
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 8);
    tft.printf("#%03d", index + 1);

    char nombreBuffer[32];
    strcpy_P(nombreBuffer, (char*)pgm_read_ptr(&(pokemon_nombres[index])));
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 28);
    tft.print(nombreBuffer);
  }

  const uint16_t *spritePtr = (const uint16_t*)pgm_read_ptr(&(pokemon_sprites[index]));

  int ancho = 96; 
  int alto = 96;  
  int startX = (tft.width() - ancho) / 2; 
  int startY = 52; 

  for (int j = 0; j < alto; j++) {
    for (int i = 0; i < ancho; i++) {
      uint16_t color = pgm_read_word(&spritePtr[j * ancho + i]);
      uint16_t colorCorregido = (color >> 8) | (color << 8);

      if (colorCorregido != 0x0000) { 
        if (silueta) {
          tft.drawPixel(startX + i, startY + j, TFT_BLACK); 
        } else {
          tft.drawPixel(startX + i, startY + j, colorCorregido); 
        }
      }
    }
  }

  if (!silueta) {
    tft.drawFastHLine(0, 153, tft.width(), TFT_DARKGREY);
    
    char tipoBuffer[32];
    strcpy_P(tipoBuffer, (char*)pgm_read_ptr(&(pokemon_tipos[index])));
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 160);
    tft.print("TIPO: ");
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.print(tipoBuffer);

    char statsBuffer[128];
    strcpy_P(statsBuffer, (char*)pgm_read_ptr(&(pokemon_stats[index])));

    char *token = strtok(statsBuffer, "|");
    int posY = 175;
    while (token != NULL) {
      tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      tft.setCursor(10, posY);
      tft.print(token);
      posY += 13; 
      token = strtok(NULL, "|");
    }
  } else {
    tft.drawRect(6, 165, tft.width() - 12, 45, TFT_BLACK);
    tft.fillRect(7, 166, tft.width() - 14, 43, TFT_BLACK);

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(0x07FF, TFT_BLACK); 
    tft.setTextSize(1);
    tft.drawString("ADIVINANDO...", tft.width() / 2, 178);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Espera la revelacion", tft.width() / 2, 194);
  }

  if (modoActual == 1) {
    tft.setTextColor(TFT_YELLOW, silueta ? 0xF800 : TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(tft.width() - 42, silueta ? 218 : 225);
    tft.print("[RND]");
  } else if (modoActual == 2) {
    tft.setTextColor(0x07FF, silueta ? 0xF800 : TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(tft.width() - 42, silueta ? 218 : 225);
    tft.print("[HOW]");
  } else if (modoActual == 3) {
    tft.setTextColor(TFT_GREEN, silueta ? 0xF800 : TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(tft.width() - 42, silueta ? 218 : 225);
    tft.print("[RUN]");
  }
}

void dibujarMenu() {
  tft.setRotation(0); 
  tft.fillScreen(0x18EF); 

  tft.drawRect(4, 4, tft.width() - 8, tft.height() - 8, TFT_DARKGREY);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_YELLOW, 0x18EF);
  tft.setTextSize(2);
  tft.drawString("CONFIG", tft.width() / 2, 12);
  tft.setTextColor(TFT_CYAN, 0x18EF);
  tft.setTextSize(1);
  tft.drawString("- OPCIONES -", tft.width() / 2, 32);

  tft.drawFastHLine(12, 45, tft.width() - 24, TFT_YELLOW);

  int yBox1 = 54;
  if (opcionMenu == 0) {
    tft.fillRect(10, yBox1, tft.width() - 20, 36, 0x02E0); 
    tft.drawRect(10, yBox1, tft.width() - 20, 36, TFT_GREEN);
  } else {
    tft.drawRect(10, yBox1, tft.width() - 20, 36, TFT_DARKGREY);
  }
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, (opcionMenu == 0) ? 0x02E0 : 0x18EF);
  tft.setTextSize(1);
  tft.setCursor(16, yBox1 + 5);
  tft.print("Modo:");
  tft.setTextColor(TFT_YELLOW, (opcionMenu == 0) ? 0x02E0 : 0x18EF);
  tft.setCursor(55, yBox1 + 5);
  if (modoActual == 0) tft.print("< MANUAL >");
  else if (modoActual == 1) tft.print("< RANDOM >");
  else if (modoActual == 2) tft.print("< WHO'S? >");
  else if (modoActual == 3) tft.print("< RUNNER >");

  tft.setTextColor(TFT_LIGHTGREY, (opcionMenu == 0) ? 0x02E0 : 0x18EF);
  tft.setCursor(16, yBox1 + 20);
  if (modoActual == 0) tft.print("Navegacion libre");
  else if (modoActual == 1) tft.print("Cambio automatico");
  else if (modoActual == 2) tft.print("Adivina la silueta");
  else if (modoActual == 3) tft.print("Minijuego Arcade + Aura");

  int yBox2 = 96;
  if (opcionMenu == 1) {
    tft.fillRect(10, yBox2, tft.width() - 20, 38, 0x02E0);
    tft.drawRect(10, yBox2, tft.width() - 20, 38, TFT_GREEN);
  } else {
    tft.drawRect(10, yBox2, tft.width() - 20, 38, TFT_DARKGREY);
  }
  tft.setTextColor(TFT_WHITE, (opcionMenu == 1) ? 0x02E0 : 0x18EF);
  tft.setCursor(16, yBox2 + 5);
  tft.print("Velocidad:");
  tft.setTextColor(TFT_YELLOW, (opcionMenu == 1) ? 0x02E0 : 0x18EF);
  tft.setCursor(80, yBox2 + 5);
  tft.printf("< %2d >", randomVelocidad);

  int barraAncho = (tft.width() - 36) * randomVelocidad / 10;
  tft.fillRect(16, yBox2 + 22, tft.width() - 32, 8, TFT_BLACK);
  tft.fillRect(16, yBox2 + 22, barraAncho, 8, TFT_GREEN);
  tft.drawRect(16, yBox2 + 22, tft.width() - 32, 8, TFT_LIGHTGREY);

  int yBox3 = 140;
  if (opcionMenu == 2) {
    tft.fillRect(10, yBox3, tft.width() - 20, 28, 0x02E0);
    tft.drawRect(10, yBox3, tft.width() - 20, 28, TFT_GREEN);
  } else {
    tft.drawRect(10, yBox3, tft.width() - 20, 28, TFT_DARKGREY);
  }
  tft.setTextColor(TFT_WHITE, (opcionMenu == 2) ? 0x02E0 : 0x18EF);
  tft.setCursor(16, yBox3 + 8);
  tft.print("> Salir");

  tft.drawFastHLine(12, 176, tft.width() - 24, TFT_DARKGREY);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_YELLOW, 0x18EF);
  tft.drawString("CONTROLES:", tft.width() / 2, 186);
  tft.setTextColor(TFT_LIGHTGREY, 0x18EF);
  tft.drawString("BTN1: Salta", tft.width() / 2, 202);
  tft.drawString("BTN2: Aura Trueno", tft.width() / 2, 218);
}

void setup() {
  Serial.begin(115200);

  preferences.begin("pokedex", false);
  highscoreRunner = preferences.getInt("highscore", 0);

  tft.init();
  tft.invertDisplay(true); 
  tft.setRotation(0); 

#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  mostrarPantallaBoot();
  mostrarPokemon(pokemonActual, false);
}

void loop() {
  unsigned long currentMillis = millis();
  bool btnPrev = (digitalRead(BTN_PREV) == LOW);
  bool btnNext = (digitalRead(BTN_NEXT) == LOW);

  if (btnPrev && btnNext) {
    unsigned long startCombo = millis();
    while (digitalRead(BTN_PREV) == LOW && digitalRead(BTN_NEXT) == LOW) {
      if (millis() - startCombo > 800) { 
        enMenu = true;
        opcionMenu = 0;
        dibujarMenu();
        delay(400); 
        break;
      }
    }
  }

  if (enMenu) {
    if (btnPrev) {
      if (opcionMenu < 2) opcionMenu++;
      else opcionMenu = 0;
      dibujarMenu();
      delay(250);
    }

    if (btnNext) {
      if (opcionMenu == 0) {
        modoActual = (modoActual + 1) % 4; 
      } 
      else if (opcionMenu == 1) {
        randomVelocidad++;
        if (randomVelocidad > 10) randomVelocidad = 1;
      } 
      else if (opcionMenu == 2) {
        enMenu = false;
        if (modoActual == 2) {
          pokemonActual = random(0, 151);
          minijuegoRevelado = false;
          tiempoFaseMinijuego = currentMillis;
          mostrarPokemon(pokemonActual, true); 
        } else if (modoActual == 3) {
          puntajeRunner = 0;
          velocidadRunner = 3.5;
          pokebolaX = 240;
          pikaY = 100;
          pikaVy = 0;
          truenosDisponibles = 2; 
          juegoTerminado = false;
          gameOverDibujado = false;
          ultimoPuntajeDibujado = -1;
          ultimosTruenosDibujados = -1;
          ultimoHighscoreDibujado = -1;
          tft.setRotation(1);
          tft.fillScreen(TFT_BLACK);
          tft.drawFastHLine(0, 18, tft.width(), TFT_DARKGREY);
        } else {
          mostrarPokemon(pokemonActual, false);
        }
        delay(300);
        return;
      }
      dibujarMenu();
      delay(250);
    }
    return; 
  }

  if (modoActual == 3) {
    ejecutarMinijuegoRunner(btnPrev, btnNext);
    return;
  }

  if (modoActual == 1) {
    int intervalo = map(randomVelocidad, 1, 10, 2000, 200);
    if (currentMillis - ultimoCambioRandom > intervalo) {
      pokemonActual = random(0, 151); 
      mostrarPokemon(pokemonActual, false);
      ultimoCambioRandom = currentMillis;
    }
    if (btnPrev || btnNext) {
      modoActual = 0; 
      mostrarPokemon(pokemonActual, false);
      delay(300);
    }
    return;
  }

  if (modoActual == 2) {
    if (!minijuegoRevelado) {
      if (currentMillis - tiempoFaseMinijuego >= 4000) {
        minijuegoRevelado = true;
        tiempoFaseMinijuego = currentMillis; 
        mostrarPokemon(pokemonActual, false);
      }
    } else {
      if (currentMillis - tiempoFaseMinijuego >= 2000) {
        pokemonActual = random(0, 151); 
        minijuegoRevelado = false;
        tiempoFaseMinijuego = currentMillis;
        mostrarPokemon(pokemonActual, true);
      }
    }
    if (btnPrev || btnNext) {
      modoActual = 0;
      minijuegoRevelado = false;
      mostrarPokemon(pokemonActual, false);
      delay(300);
    }
    return;
  }

  if (btnNext) {
    if (!btnNextWasPressed) {
      pokemonActual = (pokemonActual + 1) % 151;
      mostrarPokemon(pokemonActual, false);
      pressStartTime = currentMillis;
      lastStepTime = currentMillis;
      btnNextWasPressed = true;
    } else {
      if (currentMillis - pressStartTime > 400 && currentMillis - lastStepTime > 100) {
        pokemonActual = (pokemonActual + 1) % 151;
        mostrarPokemon(pokemonActual, false);
        lastStepTime = currentMillis;
      }
    }
  } else {
    btnNextWasPressed = false;
  }

  if (btnPrev) {
    if (!btnPrevWasPressed) {
      pokemonActual = (pokemonActual - 1 + 151) % 151;
      mostrarPokemon(pokemonActual, false);
      pressStartTime = currentMillis;
      lastStepTime = currentMillis;
      btnPrevWasPressed = true;
    } else {
      if (currentMillis - pressStartTime > 400 && currentMillis - lastStepTime > 100) {
        pokemonActual = (pokemonActual - 1 + 151) % 151;
        mostrarPokemon(pokemonActual, false);
        lastStepTime = currentMillis;
      }
    }
  } else {
    btnPrevWasPressed = false;
  }

  delay(10);
}

// --- MINIJUEGO RUNNER (CON GAME OVER ESTÁTICO Y AURA AMPLIADA DE RAYITOS) ---
void ejecutarMinijuegoRunner(bool btnPrev, bool btnNext) {
  tft.setRotation(1); 
  
  unsigned long currentMillis = millis();
  
  if (currentMillis - ultimoFrameRunner < 25) return; 
  ultimoFrameRunner = currentMillis;

  if (juegoTerminado) {
    // Dibujar la pantalla de Game Over una sola vez para evitar el parpadeo
    if (!gameOverDibujado) {
      if (puntajeRunner > highscoreRunner) {
        highscoreRunner = puntajeRunner;
        preferences.putInt("highscore", highscoreRunner);
      }

      tft.fillScreen(TFT_BLACK);
      tft.setTextDatum(TC_DATUM);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setTextSize(2);
      tft.drawString("GAME OVER", tft.width() / 2, 30);
      
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextSize(1);
      tft.setCursor(tft.width() / 2 - 45, 60);
      tft.printf("Puntaje: %d", puntajeRunner);

      tft.setCursor(tft.width() / 2 - 45, 78);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.printf("Record: %d", highscoreRunner);

      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("Presiona para reiniciar", tft.width() / 2, 105);
      
      gameOverDibujado = true;
    }

    if (btnPrev || btnNext) {
      puntajeRunner = 0;
      velocidadRunner = 3.5;
      pokebolaX = tft.width();
      truenosDisponibles = 2; 
      juegoTerminado = false;
      gameOverDibujado = false;
      ultimoPuntajeDibujado = -1;
      ultimosTruenosDibujados = -1;
      tft.fillScreen(TFT_BLACK);
      tft.drawFastHLine(0, 18, tft.width(), TFT_DARKGREY);
      delay(200);
    }
    return;
  }

  // BOTÓN 1 (BTN_PREV): SALTAR
  if (btnPrev && enSuelo) {
    pikaVy = -9; 
    enSuelo = false;
  }

  // BOTÓN 2 (BTN_NEXT): AURA TRUENO (Rango ampliado)
  static bool btnNextLock = false;
  if (btnNext) {
    if (!btnNextLock && truenosDisponibles > 0) {
      truenosDisponibles--;
      mostrandoAura = true;
      tiempoAura = currentMillis;

      // Rango de destrucción ampliado (abarca desde muy cerca hasta más adelante)
      if (pokebolaX >= 15 && pokebolaX <= 85) {
        pokebolaX = tft.width() + 20; 
        puntajeRunner++;
        if (puntajeRunner % 5 == 0 && truenosDisponibles < 2) {
          truenosDisponibles++;
        }
      }
      btnNextLock = true;
    }
  } else {
    btnNextLock = false;
  }

  // Duración del aura eléctrica activa (200ms)
  if (mostrandoAura && (currentMillis - tiempoAura > 200)) {
    mostrandoAura = false;
  }

  // Gravedad y física
  pikaVy += 1;
  pikaY += pikaVy;
  if (pikaY >= 100) {
    pikaY = 100;
    enSuelo = true;
    pikaVy = 0;
  }

  // Mover pokébola
  pokebolaX -= velocidadRunner;
  if (pokebolaX < -15) {
    pokebolaX = tft.width() + 15;
    puntajeRunner++;
    velocidadRunner += 0.35; 

    if (puntajeRunner % 5 == 0 && truenosDisponibles < 2) {
      truenosDisponibles++;
    }
  }

  // Colisión normal con la pokébola
  if (pokebolaX > 25 && pokebolaX < 45 && pikaY > 82) {
    if (mostrandoAura) {
      pokebolaX = tft.width() + 20;
      puntajeRunner++;
    } else {
      juegoTerminado = true;
    }
  }

  // --- RENDERIZADO (ZONA DE JUEGO) ---
  tft.fillRect(0, 20, tft.width(), tft.height() - 20, TFT_BLACK);

  // Suelo
  tft.drawFastHLine(0, 120, tft.width(), TFT_GREEN);
  tft.drawFastHLine(0, 121, tft.width(), TFT_DARKGREY);

  // Pikachu
  int px = 30;
  int py = pikaY;
  tft.fillRect(px + 2, py - 8, 3, 8, TFT_YELLOW);
  tft.fillRect(px + 13, py - 8, 3, 8, TFT_YELLOW);
  tft.fillRect(px + 2, py - 8, 3, 2, TFT_BLACK);
  tft.fillRect(px + 13, py - 8, 3, 2, TFT_BLACK);
  tft.fillRect(px, py, 18, 18, TFT_YELLOW);
  tft.fillRect(px + 12, py + 5, 2, 2, TFT_BLACK);
  tft.fillRect(px + 11, py + 8, 3, 3, TFT_RED);
  tft.fillRect(px - 4, py + 8, 4, 3, TFT_YELLOW);

  // EFECTO VISUAL DE AURA TRUENO MEJORADA (Rayitos eléctricos con más rango)
  if (mostrandoAura) {
    // Círculos concéntricos de mayor tamaño
    tft.drawCircle(px + 9, py + 9, 20, TFT_CYAN);
    tft.drawCircle(px + 9, py + 9, 16, TFT_YELLOW);
    // Rayitos múltiples mejor representados (zigzag y chispas extendidas)
    tft.drawLine(px - 10, py - 5, px - 2, py + 2, TFT_WHITE);
    tft.drawLine(px - 2, py + 2, px - 8, py + 12, TFT_CYAN);
    tft.drawLine(px + 25, py - 2, px + 32, py + 6, TFT_YELLOW);
    tft.drawLine(px + 32, py + 6, px + 26, py + 14, TFT_WHITE);
    tft.drawLine(px + 5, py - 14, px + 12, py - 8, TFT_CYAN);
    tft.drawLine(px + 12, py - 8, px + 18, py - 15, TFT_YELLOW);
  }

  // Pokébola
  if (pokebolaX < tft.width()) {
    int pbX = (int)pokebolaX;
    int pbY = 110;
    tft.fillCircle(pbX, pbY, 9, TFT_RED);
    tft.drawCircle(pbX, pbY, 9, TFT_BLACK);
    tft.drawFastHLine(pbX - 9, pbY, 18, TFT_BLACK);
    tft.fillCircle(pbX, pbY, 3, TFT_WHITE);
    tft.drawCircle(pbX, pbY, 3, TFT_BLACK);
  }

  // --- BARRA SUPERIOR ESTÁTICA ---
  if (puntajeRunner != ultimoPuntajeDibujado || highscoreRunner != ultimoHighscoreDibujado) {
    tft.fillRect(5, 2, 130, 15, TFT_BLACK); 
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.printf("PT:%d HI:%d", puntajeRunner, highscoreRunner);
    ultimoPuntajeDibujado = puntajeRunner;
    ultimoHighscoreDibujado = highscoreRunner;
  }

  if (truenosDisponibles != ultimosTruenosDibujados) {
    tft.fillRect(140, 2, 90, 15, TFT_BLACK); 
    tft.setCursor(145, 5);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.printf("TRUENO:%d/2", truenosDisponibles);
    ultimosTruenosDibujados = truenosDisponibles;
    
    tft.drawFastHLine(0, 18, tft.width(), TFT_DARKGREY);
  }
}