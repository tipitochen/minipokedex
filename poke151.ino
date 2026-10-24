#include <SPI.h>
#include <TFT_eSPI.h>
#include "pokemon_data.h"//151

TFT_eSPI tft = TFT_eSPI();

// Pines de los botones
#define BTN_PREV 0   // Botón 1 (GPIO 0)
#define BTN_NEXT 35  // Botón 2 (GPIO 35)

int pokemonActual = 0; 

// Variables para control de velocidad al mantener en modo manual
unsigned long pressStartTime = 0;
unsigned long lastStepTime = 0;
bool btnNextWasPressed = false;
bool btnPrevWasPressed = false;

// Variables del Menú y Modos (0: Manual, 1: Random, 2: Who's That Pokémon)
bool enMenu = false;
int modoActual = 0;      // 0: Manual, 1: Random, 2: Who's That Pokémon
int randomVelocidad = 5; // Velocidad para el modo random (1 a 10)
int opcionMenu = 0;      // 0: Modo, 1: Velocidad, 2: Salir
unsigned long ultimoCambioRandom = 0;

// Variables específicas para el Minijuego "Who's That Pokémon?"
bool minijuegoRevelado = false;
unsigned long tiempoFaseMinijuego = 0;

void mostrarPantallaBoot() {
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
  if (silueta) {
    // Fondo Rojo estilo la imagen de referencia (RGB565: 0xF800)
    tft.fillScreen(0xF800); 

    // Dibujamos un fondo circular blanco central estilo "destello" para que resalte la silueta negra
    int centerX = tft.width() / 2;
    int centerY = 100;
    tft.fillCircle(centerX, centerY, 62, TFT_WHITE);
    tft.drawCircle(centerX, centerY, 62, 0x07FF); // Borde cian/turquesa

    // Franja superior de lado a lado para el título
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

  // 2. RENDERIZADO DEL SPRITE CENTRAL (96x96)
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
          tft.drawPixel(startX + i, startY + j, TFT_BLACK); // Sombra negra pura y sólida
        } else {
          tft.drawPixel(startX + i, startY + j, colorCorregido); 
        }
      }
    }
  }

  // 3. PANEL INFERIOR
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
    tft.setTextColor(0x07FF, TFT_BLACK); // Cian brillante
    tft.setTextSize(1);
    tft.drawString("ADIVINANDO...", tft.width() / 2, 178);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Espera la revelacion", tft.width() / 2, 194);
  }

  // Indicador de modo abajo a la derecha
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
  }
}

void dibujarMenu() {
  tft.fillScreen(0x18EF); 

  tft.drawRect(4, 4, tft.width() - 8, tft.height() - 8, TFT_DARKGREY);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_YELLOW, 0x18EF);
  tft.setTextSize(2);
  tft.drawString("CONFIG", tft.width() / 2, 12);
  tft.setTextColor(TFT_CYAN, 0x18EF);
  tft.setTextSize(1);
  tft.drawString("- OPCIONES DE JUEGO -", tft.width() / 2, 32);

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
  tft.setCursor(60, yBox1 + 5);
  if (modoActual == 0) tft.print("< MANUAL >");
  else if (modoActual == 1) tft.print("< RANDOM >");
  else if (modoActual == 2) tft.print("< WHO'S? >");

  tft.setTextColor(TFT_LIGHTGREY, (opcionMenu == 0) ? 0x02E0 : 0x18EF);
  tft.setCursor(16, yBox1 + 20);
  if (modoActual == 0) tft.print("Navegacion libre");
  else if (modoActual == 1) tft.print("Cambio automatico");
  else if (modoActual == 2) tft.print("Adivina la silueta");

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
  tft.setCursor(85, yBox2 + 5);
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
  tft.drawString("BTN1: Mover", tft.width() / 2, 202);
  tft.drawString("BTN2: OK / Cambiar", tft.width() / 2, 218);
}

void setup() {
  Serial.begin(115200);

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
      if (opcionMenu < 2) {
        opcionMenu++;
      } else {
        opcionMenu = 0;
      }
      dibujarMenu();
      delay(250);
    }

    if (btnNext) {
      if (opcionMenu == 0) {
        modoActual = (modoActual + 1) % 3;
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
      if (currentMillis - pressStartTime > 400) {
        if (currentMillis - lastStepTime > 100) {
          pokemonActual = (pokemonActual + 1) % 151;
          mostrarPokemon(pokemonActual, false);
          lastStepTime = currentMillis;
        }
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
      if (currentMillis - pressStartTime > 400) {
        if (currentMillis - lastStepTime > 100) {
          pokemonActual = (pokemonActual - 1 + 151) % 151;
          mostrarPokemon(pokemonActual, false);
          lastStepTime = currentMillis;
        }
      }
    }
  } else {
    btnPrevWasPressed = false;
  }

  delay(10);
}