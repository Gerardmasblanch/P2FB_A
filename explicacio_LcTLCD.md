# Explicació completa del TAD LCD (LcTLCD)

Controlador: HD44780 en mode 4 bits.
Timing: 1 tic = 416 µs.

---

## Arquitectura en capes

```
Aplicació
    │  LcInit / LcClear / LcGotoXY / LcPutChar / LcPutString / LcCursorOn / LcCursorOff
    ▼
Capa lògica         → sap on és el cursor, quantes files/columnes té el display
    │  CantaIR / CantaData
    ▼
Capa de protocol    → genera els nibbles i els polsos Enable correctes
    │  CantaPartAlta / CantaPartBaixa
    ▼
Capa física         → escriu directament als registres LATB / TRISB del microcontrolador
```

---

## Connexions hardware (definides al .h)

| Senyal LCD | Pin PIC | Funció |
|-----------|---------|--------|
| RS        | RB3     | 0 = ordre (IR), 1 = dada (DR) |
| R/W       | RB15    | 0 = escriptura, 1 = lectura |
| E         | RB5     | pols d'activació (flanc descendent) |
| D4        | RB6     | bit de dades (nibble baix) |
| D5        | RB7     | bit de dades |
| D6        | RB8     | bit de dades |
| D7        | RB9     | bit de dades / busy flag en lectura |

El LCD usa mode 4 bits: cada byte s'envia en dos nibbles, primer el alt (D7-D4), després el baix (D3-D0). Cada nibble requereix un pols Enable.

---

## Constants de protocol

| Constant | Valor | Significat |
|----------|-------|-----------|
| `FUNCTION_SET` | 0x20 | Selecciona mode: bits, files, font |
| `BITS_8` | 0x10 | OR amb FUNCTION_SET → mode 8 bits (durant init) |
| `DISPLAY_CONTROL` | 0x08 | Base de la comanda de control del display |
| `DISPLAY_ON` | 0x04 | OR → display encès |
| `CURSOR_ON` | 0x02 | OR → cursor visible |
| `DISPLAY_CLEAR` | 0x01 | Esborra tot el display (triga fins a 1,64 ms) |
| `ENTRY_MODE` | 0x04 | Avança cursor automàticament cap a la dreta |
| `SET_DDRAM` | 0x80 | Base per moure el cursor (OR amb adreça física) |

---

## Variables estàtiques internes

| Variable | Tipus | Conté |
|----------|-------|-------|
| `Rows` | unsigned char | Nombre de files (1, 2 o 4) |
| `Columns` | unsigned char | Nombre de columnes (8–40) |
| `RowAct` | unsigned char | Fila actual del cursor (0-based) |
| `ColumnAct` | unsigned char | Columna actual del cursor (0-based) |
| `Timer` | int | Handle del timer reservat al TiTTimer |
| `LcdEstat` | unsigned char | Estat actual de la màquina cooperativa |
| `LcdOp` | unsigned char | Operació destí (on anar quan busy=0) |
| `LcdCharPendent` | unsigned char | Argument pendent: caràcter o columna |
| `LcdRowPendent` | unsigned char | Argument pendent: fila (per LcGotoXY) — cal afegir-la |
| `LcdStringPendent` | char* | Punter a la cadena pendent (LcPutString) |

---

## Funcions privades de capa física

### `CantaPartAlta(char c)`

Posa els 4 bits alts de `c` (bits 7-4) als pins D7-D4 del port B.
No genera cap pols Enable. Simplement carrega el valor als latchs.

### `CantaPartBaixa(char c)`

Posa els 4 bits baixos de `c` (bits 3-0) als pins D7-D4 del port B.
Igual que CantaPartAlta però agafa la meitat baixa.

Aquestes dues funcions no saben si envien una ordre o una dada — simplement posen els bits als pins.

---

## Funcions privades de capa de protocol

### `CantaIR(char IR)` — enviar ordre (Instruction Register)

Seqüència:
1. Configura D4-D7 com a sortides
2. RS = 0 (és una ordre, no una dada)
3. RW = 0 (escriptura)
4. Enable = 1 (prepara pols)
5. Carrega nibble ALT de `IR` a D4-D7
6. Enable = 1 (assegura amplada de pols ≥500 ns)
7. Enable = 0 → flanc descendent: el LCD llegeix el nibble alt
8. Enable = 0 (assegura amplada baixa del pols ≥230 ns)
9. Enable = 1 (prepara segon pols)
10. Carrega nibble BAIX de `IR` a D4-D7
11. Enable = 1 (assegura ≥500 ns)
12. Enable = 0 → flanc descendent: el LCD llegeix el nibble baix
13. Configura D4-D7 com a entrades

El LCD interpreta els dos nibbles junts com un byte de comanda complet.

### `CantaData(char Data)` — enviar dada (Data Register)

Idèntica a `CantaIR` però amb RS = 1 (indica que és una dada, no una ordre).
El LCD escriu el caràcter a la posició actual del cursor i avança automàticament el cursor.

### `EscriuPrimeraOrdre(char ordre)`

Durant la inicialització, el LCD no sap encara si estem en mode 4 o 8 bits. Per inicialitzar-lo al mode 4 bits, cal enviar les primeres ordres com si fossin en mode 8 bits (un sol nibble, sense el segon). Aquesta funció fa exactament això:
- RS = 0, RW = 0
- Enable = 1 → carrega nibble
- Enable = 0 → el LCD llegeix

Els bits 7-4 d'`ordre` s'ignoren perquè D7-D4 físics corresponen a D3-D0 del LCD en mode 8 bits.

---

## Funcions privades de temporització (bloquejants)

### `Espera(int Timer, int ms)`

Reseteja el timer i espera en un `while` fins que hagin passat `ms` tics.
Amb 1 tic = 416 µs:

| Crida | Temps |
|-------|-------|
| `Espera(Timer, 1)` | ≥416 µs |
| `Espera(Timer, 5)` | ≥2,08 ms |
| `Espera(Timer, 100)` | ≥41,6 ms |

Bloquejant: no retorna fins que s'ha complert l'espera. No es pot usar a la versió cooperativa.

### `WaitForBusy(void)`

El LCD té un bit busy (D7 en mode lectura) que indica que està processant l'ordre anterior. Mentre busy=1, no es pot enviar cap comanda nova.

Seqüència de lectura busy flag (en mode 4 bits):
1. D4-D7 com a entrades
2. RS = 0, RW = 1 (lectura del IR)
3. Enable = 1 → presenta el nibble alt al bus
4. Llegeix `GetBusyFlag()` = D7 = bit busy
5. Enable = 0
6. Enable = 1 → presenta el nibble baix (adreça del cursor, no ens interessa)
7. Enable = 0
8. Si TiGetTics(Timer) > 0 (≥416 µs) → timeout: l'LCD ha fallat, surt igualment
9. Si busy=1 → torna al pas 3

El timeout evita un bucle infinit si l'LCD no respon. Amb 1 tic = 416 µs, el timeout és generós: l'LCD hauria de respondre en <40 µs.

Bloquejant: fa `do...while(Busy)`. A la versió cooperativa, substituïda pels estats `LCD_BUSY_SETUP` + `LCD_BUSY_POLLING` de LcMotor.

---

## Funcions públiques

### `LcInit(char rows, char columns)`

La inicialització del HD44780 és una seqüència molt específica descrita al datasheet. El motiu per fer-la dues vegades en un `for(i=0;i<2;i++)` és que la inicialització per software no sempre funciona a la primera (depèn del temps de pujada de VCC). Fer-la dues vegades garanteix que arrenca bé tant amb reset com amb power-on.

Seqüència per cada iteració:
1. `Espera(100)` → 41,6 ms d'espera inicial (l'LCD necessita estabilitzar-se)
2. Tres cops `EscriuPrimeraOrdre(CURSOR_ON | DISPLAY_CLEAR)` en mode 8 bits (repetir és part del protocol de reset del HD44780)
3. `EscriuPrimeraOrdre(CURSOR_ON)` → commuta a mode 4 bits
4. `Espera(1)` entre cada ordre
5. `CantaIR(FUNCTION_SET | DISPLAY_CONTROL)` → configura 4 bits, 1 fila, font 5x7
6. A partir d'aquí ja es pot usar WaitForBusy
7. Display OFF → Clear (+ Espera 3 tics = 1,25 ms) → Display ON amb cursor → Display ON complet

Reserva un timer del sistema (`TiGetTimer`). El timer és necessari per tots els Espera i WaitForBusy posteriors.

NOT OK a la versió cooperativa: conté `for`, múltiples `Espera` i `WaitForBusy`. La cooperativització de LcInit és el cas més complex perquè la seqüència d'init no és un simple "comprova busy → fes acció".

### `LcEnd(void)`

Allibera el timer amb `TiCloseTimer`. No fa res amb el hardware del LCD. Simple i correcte tal com està.

### `LcClear(void)`

Envia la comanda `DISPLAY_CLEAR` (0x01). Aquesta comanda:
- Escriu espais a totes les posicions de la memòria DDRAM
- Torna el cursor a (0,0)
- Triga fins a 1,64 ms (molt més que les 40 µs habituals)

Per això, a més de WaitForBusy abans d'enviar, cal un `Espera(Timer, 3)` després (3 × 416 µs = 1,25 ms) per assegurar que l'LCD ha acabat.

Versió cooperativa: `LcdOp = LCD_OP_CLEAR_ENVIA`, `LcdEstat = LCD_BUSY_SETUP`.
LcMotor ho processa: BUSY → envia DISPLAY_CLEAR → espera 3 tics → IDLE.

### `LcCursorOn(void)` / `LcCursorOff(void)`

Envien la comanda `DISPLAY_CONTROL`:
- ON:  `DISPLAY_CONTROL | DISPLAY_ON | CURSOR_ON` = 0x08 | 0x04 | 0x02 = 0x0E
- OFF: `DISPLAY_CONTROL | DISPLAY_ON`              = 0x08 | 0x04       = 0x0C

El display continua encès en tots dos casos, només canvia la visibilitat del cursor.
Triga ≤40 µs → un WaitForBusy és suficient.

Versió cooperativa: `LcdOp = LCD_OP_CURSOR_ON/OFF`, `LcdEstat = LCD_BUSY_SETUP`.

### `LcGotoXY(char Column, char Row)`

Mou el cursor a una posició lògica (columna, fila). El LCD no entén posicions lògiques directament — cal calcular l'adreça física de la DDRAM:

| Display | Fila | Adreça física |
|---------|------|--------------|
| 1 fila  | 0    | `Column` |
| 2 files | 0    | `Column` |
| 2 files | 1    | `Column + 0x40` |
| 4 files | 0    | `Column` |
| 4 files | 1    | `Column + 0x40` |
| 4 files | 2    | `Column + Columns` |
| 4 files | 3    | `Column + 0x40 + Columns` |

Les files 0 i 2 estan en memòria contigua des de 0x00. Les files 1 i 3 comencen a 0x40. El salt de 0x40 és una característica del HD44780 independent de la posició visual.

La comanda és `SET_DDRAM | adreça_fisica` (0x80 OR l'adreça).
Actualitza `RowAct` i `ColumnAct` amb els nous valors.

Versió cooperativa: `LcdCharPendent=Column`, `LcdRowPendent=Row`, `LcdOp=LCD_OP_GOTOXY`, `LcdEstat=LCD_BUSY_SETUP`. LcMotor calcula Fisics i fa CantaIR directament (sense cridar la funció pública).

### `LcPutChar(char c)`

Escriu un caràcter a la posició actual del cursor. El LCD usa el codi ASCII estàndard.

Seqüència:
1. WaitForBusy → CantaData(c) → el LCD escriu el caràcter i avança el cursor automàticament
2. Incrementa `ColumnAct`
3. Si `ColumnAct` arriba al límit de fila → wrap:
   - Reseteja ColumnAct a 0, avança RowAct
   - Crida LcGotoXY per reposicionar el cursor (el cursor LCD hauria avançat sol però pot haver saltat a una adreça no contigua visualment)

El wrap depèn del tipus de display:
- 4 files (`Rows==3` al codi, possible typo de la versió original): límit a columna 20
- 2 files: límit a columna 40
- 1 fila: límit a columna 40, no avança fila

Versió cooperativa: `LcdCharPendent=c`, `LcdOp=LCD_OP_PUTCHAR`, `LcdEstat=LCD_BUSY_SETUP`.

### `LcPutString(char *s)`

Iteració simple sobre la cadena: crida `LcPutChar` per cada caràcter fins al `\0`.
A la versió original fa `while(*s) LcPutChar(*s++)` — completament bloquejant.

Versió cooperativa: `LcdStringPendent=s`, `LcdOp=LCD_OP_PUTSTRING`, `LcdEstat=LCD_BUSY_SETUP`. LcMotor escriu un caràcter per crida, verificant busy abans de cadascun.

---

## LcMotor — la màquina d'estats cooperativa

LcMotor s'ha de cridar des del bucle principal del sistema a cada tick. Cada crida avança la màquina un pas i retorna immediatament.

### Constants d'estat

| Constant | Valor | Quan s'usa |
|----------|-------|-----------|
| `LCD_IDLE` | 0 | No hi ha operació en curs |
| `LCD_BUSY_SETUP` | 1 | Configurar pins per llegir busy flag |
| `LCD_BUSY_POLLING` | 2 | Llegir busy flag i esperar |
| `LCD_OP_CLEAR_ENVIA` | 3 | Enviar comanda DISPLAY_CLEAR |
| `LCD_OP_CLEAR_ESPERA` | 4 | Esperar 3 tics després de CLEAR |
| `LCD_OP_CURSOR_ON` | 5 | Enviar comanda cursor ON |
| `LCD_OP_CURSOR_OFF` | 6 | Enviar comanda cursor OFF |
| `LCD_OP_GOTOXY` | 7 | Calcular adreça DDRAM i enviar |
| `LCD_OP_PUTCHAR` | 8 | Enviar byte de dada (CantaData) |
| `LCD_OP_PUTCHAR_2` | 9 | Actualitzar posició del cursor |
| `LCD_OP_PUTSTRING` | 10 | Enviar el pròxim caràcter de la cadena |

### Diagrama de flux

```
Petició pública (ex: LcPutChar('A'))
    → LcdCharPendent = 'A'
    → LcdOp = LCD_OP_PUTCHAR
    → LcdEstat = LCD_BUSY_SETUP
    → return

─────────────────────────────────────────────────────────
Cada crida a LcMotor():

  LCD_IDLE
      └─ no fa res

  LCD_BUSY_SETUP
      ├─ D4-D7 com a entrades
      ├─ RS=0, RW=1
      ├─ TiResetTics(Timer)
      └─ LcdEstat = LCD_BUSY_POLLING

  LCD_BUSY_POLLING
      ├─ genera pols Enable i llegeix D7 (busy flag)
      ├─ si TiGetTics(Timer) > 0  [≥416 µs → timeout]
      │       └─ LcdEstat = LcdOp  (avança igualment)
      │          break
      ├─ si busy = 0
      │       └─ LcdEstat = LcdOp
      └─ si busy = 1  → return  (torna al pròxim tick)

  LCD_OP_CLEAR_ENVIA
      ├─ CantaIR(DISPLAY_CLEAR)
      ├─ TiResetTics(Timer)
      └─ LcdEstat = LCD_OP_CLEAR_ESPERA

  LCD_OP_CLEAR_ESPERA
      ├─ si TiGetTics < 3  → return  (espera 3 × 416 µs = 1,25 ms)
      └─ LcdEstat = LCD_IDLE

  LCD_OP_CURSOR_ON
      ├─ CantaIR(DISPLAY_CONTROL | DISPLAY_ON | CURSOR_ON)
      └─ LcdEstat = LCD_IDLE

  LCD_OP_CURSOR_OFF
      ├─ CantaIR(DISPLAY_CONTROL | DISPLAY_ON)
      └─ LcdEstat = LCD_IDLE

  LCD_OP_GOTOXY
      ├─ calcula Fisics (adreça DDRAM) a partir de LcdCharPendent i LcdRowPendent
      ├─ CantaIR(SET_DDRAM | Fisics)
      ├─ RowAct = LcdRowPendent, ColumnAct = LcdCharPendent
      └─ LcdEstat = LCD_IDLE  (o continua a PUTSTRING si ve d'un wrap)

  LCD_OP_PUTCHAR
      ├─ CantaData(LcdCharPendent)
      └─ LcdEstat = LCD_OP_PUTCHAR_2

  LCD_OP_PUTCHAR_2
      ├─ ColumnAct++
      ├─ si wrap de fila:
      │       ├─ actualitza ColumnAct i RowAct
      │       ├─ LcdCharPendent = ColumnAct, LcdRowPendent = RowAct
      │       ├─ LcdOp = LCD_OP_GOTOXY  (o un estat que torni a PUTSTRING)
      │       └─ LcdEstat = LCD_BUSY_SETUP
      ├─ si no wrap i ve de PUTCHAR  → LcdEstat = LCD_IDLE
      └─ si no wrap i ve de PUTSTRING → LcdEstat = LCD_BUSY_SETUP

  LCD_OP_PUTSTRING
      ├─ si *LcdStringPendent != '\0':
      │       ├─ CantaData(*LcdStringPendent++)
      │       └─ LcdEstat = LCD_OP_PUTCHAR_2
      └─ si '\0': LcdEstat = LCD_IDLE
```

### Per què LCD_BUSY_SETUP i LCD_BUSY_POLLING són dos estats separats?

`LCD_BUSY_SETUP` configura els pins i reseteja el timer. Separar-los permet que la configuració dels pins i el reset del timer passin en un tick, i la primera lectura del busy flag en el tick següent. Garanteix que el timer s'inicialitza abans de la primera lectura.

### Per què `LCD_OP_PUTCHAR_2` és un estat separat de `LCD_OP_PUTCHAR`?

Escriure el caràcter (`CantaData`) i actualitzar la posició del cursor no és una operació atòmica. Si el cursor fa wrap de fila, cal un nou busy-check abans del GotoXY. Separat en dos estats, cada crida a LcMotor fa exactament una cosa.

### Seqüència completa per `LcPutString("Hi")`

```
LcPutString("Hi") → LcdStringPendent="Hi", LcdOp=PUTSTRING, LcdEstat=BUSY_SETUP

Tick  1: BUSY_SETUP    → configura pins, TiResetTics → LcdEstat=BUSY_POLLING
Tick  2: BUSY_POLLING  → busy=0 → LcdEstat=PUTSTRING
Tick  3: PUTSTRING     → CantaData('H'), LcdStringPendent->"i" → LcdEstat=PUTCHAR_2
Tick  4: PUTCHAR_2     → ColumnAct++, no wrap, LcdOp==PUTSTRING → LcdEstat=BUSY_SETUP
Tick  5: BUSY_SETUP    → configura pins, TiResetTics → LcdEstat=BUSY_POLLING
Tick  6: BUSY_POLLING  → busy=0 → LcdEstat=PUTSTRING
Tick  7: PUTSTRING     → CantaData('i'), LcdStringPendent->"\0" → LcdEstat=PUTCHAR_2
Tick  8: PUTCHAR_2     → ColumnAct++, no wrap, LcdOp==PUTSTRING → LcdEstat=BUSY_SETUP
Tick  9: BUSY_SETUP    → configura pins, TiResetTics → LcdEstat=BUSY_POLLING
Tick 10: BUSY_POLLING  → busy=0 → LcdEstat=PUTSTRING
Tick 11: PUTSTRING     → *LcdStringPendent=='\0' → LcdEstat=IDLE
```

Total: ~11 ticks × 416 µs ≈ 4,6 ms per escriure "Hi" sense bloquejar mai el sistema.

---

## Errors a la versió actual (P2FB)

| Ubicació | Problema |
|----------|---------|
| Línia 68 | Falta `LcdRowPendent` com a variable estàtica |
| Línia 149 | `LcGotoXY(LcdCharPendent, LcdOp)`: LcdOp no és la fila, i crida la versió bloquejant |
| Línies 163, 170, 175 | `LCD_OP_PUTCHAR_2` crida `LcGotoXY` directament (bloquejant) |
| Línia 177 | `LCD_IDLE` incondicional: quan ve de PUTSTRING no continua escrivint |
| Línies 237–328 | LcClear, LcCursorOn/Off, LcGotoXY, LcPutChar, LcPutString segueixen bloquejant |
| Línies 339–342 | `Espera` segueix bloquejant |
| Línies 395–411 | `WaitForBusy` segueix bloquejant |
| `LcInit` | Cas especial: necessita tractament diferent per la seqüència d'init |