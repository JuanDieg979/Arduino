#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Inicializa la pantalla en la dirección estándar 0x27 (16 columnas x 2 filas)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- ASIGNACIÓN DE PINES (RECOMENDADOS PARA UNO R4) ---
const int pinBit0 = 2;   // Botón para introducir '0'
const int pinBit1 = 3;   // Botón para introducir '1'
const int pinSuma = 4;   // Botón de Suma (+)
const int pinResta = 5;  // Botón de Resta (-)
const int pinMult  = 6;  // Botón de Multiplicación (*)
const int pinDiv   = 7;  // Botón de División (/)
const int pinIgual = 8;  // Botón de Igual (=)
const int pinReset = 9;  // Botón de Reset (C)

// --- VARIABLES DE CONTROL DE ESTADO ---
uint8_t primerNumero = 0;
uint8_t segundoNumero = 0;
uint8_t bitsIngresados = 0; 
char operacionSeleccionada = ' ';
bool ingresandoSegundoNumero = false;

// Control de antirebote por software (Debounce)
unsigned long ultimoTiempoDebounce = 0;
const unsigned long delayDebounce = 250; 

// --- PROTOTIPOS DE OPERACIONES MATEMÁTICAS ---
uint32_t ejecutarSuma(uint32_t a, uint32_t b);
int32_t  ejecutarResta(uint32_t a, uint32_t b);
uint32_t ejecutarMultiplicacion(uint32_t a, uint32_t b);
uint32_t ejecutarDivision(uint32_t a, uint32_t b, bool &errorDivision);
void imprimir8Bits(uint8_t numero);

void setup() {
  // Configuración de pines de entrada digitales
  pinMode(pinBit0, INPUT);
  pinMode(pinBit1, INPUT);
  pinMode(pinSuma, INPUT);
  pinMode(pinResta, INPUT);
  pinMode(pinMult, INPUT);
  pinMode(pinDiv, INPUT);
  pinMode(pinIgual, INPUT);
  pinMode(pinReset, INPUT);

  // Inicialización de la pantalla LCD I2C
  lcd.init();
  lcd.backlight();
  mostrarPantallaInicial();
}

void loop() {
  // Verificación prioritaria: Botón Reset (C) en cualquier momento
  if (digitalRead(pinReset) == HIGH) {
    reiniciarCalculadora();
    delay(delayDebounce);
    return;
  }

  // Filtrado de rebotes para asegurar lecturas mecánicas limpias
  if ((millis() - ultimoTiempoDebounce) > delayDebounce) {
    
    // --- LECTURA DE ENTRADA DE BITS DE DERECHA A IZQUIERDA ---
    if (digitalRead(pinBit0) == HIGH || digitalRead(pinBit1) == HIGH) {
      if (bitsIngresados < 8) {
        uint8_t bitLeido = (digitalRead(pinBit1) == HIGH) ? 1 : 0;
        
        if (!ingresandoSegundoNumero) {
          primerNumero |= (bitLeido << bitsIngresados);
          bitsIngresados++;
          actualizarDisplayBinario(primerNumero);
        } else {
          segundoNumero |= (bitLeido << bitsIngresados);
          bitsIngresados++;
          actualizarDisplayBinario(segundoNumero);
        }
      } else {
        // Alerta visual si intentas meter un 9no bit
        lcd.noBlink(); 
        lcd.setCursor(15, (ingresandoSegundoNumero ? 1 : 0));
        lcd.print("!");
        delay(200);
        lcd.setCursor(15, (ingresandoSegundoNumero ? 1 : 0));
        lcd.print(" ");
        lcd.blink(); 
      }
      ultimoTiempoDebounce = millis();
    }

    // --- SELECCIÓN DE LA OPERACIÓN (+, -, *, /) ---
    if (!ingresandoSegundoNumero && bitsIngresados > 0) {
      if (digitalRead(pinSuma) == HIGH)       { operacionSeleccionada = '+'; iniciarSegundoNumero(); }
      else if (digitalRead(pinResta) == HIGH) { operacionSeleccionada = '-'; iniciarSegundoNumero(); }
      else if (digitalRead(pinMult) == HIGH)  { operacionSeleccionada = '*'; iniciarSegundoNumero(); }
      else if (digitalRead(pinDiv) == HIGH)   { operacionSeleccionada = '/'; iniciarSegundoNumero(); }
    }

    // --- PROCESAMIENTO DEL RESULTADO (=) ---
    if (ingresandoSegundoNumero && bitsIngresados > 0 && digitalRead(pinIgual) == HIGH) {
      procesarResultado();
      ultimoTiempoDebounce = millis();
    }
  }
}

// --- LOGICA DE INTERFAZ DE USUARIO ---

void mostrarPantallaInicial() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calculadora Bin");
  lcd.setCursor(0, 1);
  lcd.print("Listo: Ingrese N1");
  delay(1500); 
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("N1: 00000000");
  
  // Cursor parpadeante sobre el primer bit editable (extrema derecha)
  lcd.setCursor(11, 0);
  lcd.cursor();
  lcd.blink();
}

// Función auxiliar para imprimir los 8 bits en orden natural (MSB a LSB)
void imprimir8Bits(uint8_t numero) {
  for (int i = 7; i >= 0; i--) {
    lcd.print((numero >> i) & 1);
  }
}

void actualizarDisplayBinario(uint8_t numero) {
  if (!ingresandoSegundoNumero) {
    lcd.setCursor(0, 0);
    lcd.print("N1: ");
    imprimir8Bits(numero);
    
    // Mueve el cursor parpadeante hacia la izquierda
    if (bitsIngresados < 8) {
      lcd.setCursor(11 - bitsIngresados, 0);
    } else {
      lcd.noBlink(); 
    }
  } else {
    lcd.setCursor(0, 1);
    lcd.print(operacionSeleccionada);
    lcd.print("  ");
    imprimir8Bits(numero);
    
    // Mueve el cursor en la línea inferior
    if (bitsIngresados < 8) {
      lcd.setCursor(11 - bitsIngresados, 1);
    } else {
      lcd.noBlink();
    }
  }
}

void iniciarSegundoNumero() {
  lcd.noCursor();
  lcd.noBlink();
  lcd.clear(); 
  
  lcd.setCursor(0, 0);
  lcd.print("N1: ");
  imprimir8Bits(primerNumero);
  
  lcd.setCursor(0, 1);
  lcd.print(operacionSeleccionada);
  lcd.print("  00000000"); 
  
  bitsIngresados = 0; 
  ingresandoSegundoNumero = true;
  lcd.setCursor(11, 1);
  lcd.cursor();
  lcd.blink();
  
  ultimoTiempoDebounce = millis();
}

void reiniciarCalculadora() {
  lcd.noCursor();
  lcd.noBlink();
  primerNumero = 0;
  segundoNumero = 0;
  bitsIngresados = 0;
  operacionSeleccionada = ' ';
  ingresandoSegundoNumero = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reseteando...");
  delay(600);
  mostrarPantallaInicial();
}

void procesarResultado() {
  lcd.noCursor();
  lcd.noBlink();
  lcd.clear();
  
  bool errorDivision = false;
  int32_t resultadoFinal = 0;
  bool esNegativo = false;

  switch (operacionSeleccionada) {
    case '+': resultadoFinal = ejecutarSuma(primerNumero, segundoNumero); break;
    case '-': 
      resultadoFinal = ejecutarResta(primerNumero, segundoNumero); 
      if (resultadoFinal < 0) esNegativo = true;
      break;
    case '*': resultadoFinal = ejecutarMultiplicacion(primerNumero, segundoNumero); break;
    case '/': resultadoFinal = ejecutarDivision(primerNumero, segundoNumero, errorDivision); break;
  }

  if (errorDivision) {
    lcd.setCursor(0, 0);
    lcd.print("Error:");
    lcd.setCursor(0, 1);
    lcd.print("Division por 0");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("BIN: ");
  if (esNegativo) {
    lcd.print("-");
    resultadoFinal = -resultadoFinal;
  }
  lcd.print(resultadoFinal, BIN);

  lcd.setCursor(0, 1);
  lcd.print("DEC: ");
  if (esNegativo) lcd.print("-");
  lcd.print(resultadoFinal);
}

// --- BLOQUE MATEMÁTICO PURO OPTIMIZADO EN ENSAMBLADOR ARM CORTEX-M4 ---
// El compilador de Arduino traduce este bloque directamente a código máquina nativo

uint32_t ejecutarSuma(uint32_t a, uint32_t b) {
  uint32_t resultado;
  __asm__ __volatile__ (
    "add %0, %1, %2"      // Suma el registro de 'a' (%1) y 'b' (%2) -> guarda en 'resultado' (%0)
    : "=r" (resultado)    
    : "r" (a), "r" (b)    
  );
  return resultado;
}

int32_t ejecutarResta(uint32_t a, uint32_t b) {
  int32_t resultado;
  __asm__ __volatile__ (
    "sub %0, %1, %2"      // Resta 'a' (%1) menos 'b' (%2) -> guarda en 'resultado' (%0)
    : "=r" (resultado)
    : "r" (a), "r" (b)
  );
  return resultado;
}

uint32_t ejecutarMultiplicacion(uint32_t a, uint32_t b) {
  uint32_t resultado;
  __asm__ __volatile__ (
    "mul %0, %1, %2"      // Multiplica 'a' (%1) por 'b' (%2) -> guarda en 'resultado' (%0)
    : "=r" (resultado)
    : "r" (a), "r" (b)
  );
  return resultado;
}

uint32_t ejecutarDivision(uint32_t a, uint32_t b, bool &errorDivision) {
  if (b == 0) { 
    errorDivision = true; 
    return 0; 
  }
  errorDivision = false;

  uint32_t resultado;
  __asm__ __volatile__ (
    "udiv %0, %1, %2"     // División sin signo de 'a' (%1) entre 'b' (%2) -> guarda en 'resultado' (%0)
    : "=r" (resultado)
    : "r" (a), "r" (b)
  );
  return resultado;
}