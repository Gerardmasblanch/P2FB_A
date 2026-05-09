# Esquema funcional del TAD_FARM

`TAD_FARM` és el **coordinador central** del sistema embedded. No parla directament amb el hardware: orquestra la resta de TADs (`TAD_LCD`, `TAD_SERIAL`, `POLS`, `ADC`, etc.), manté l'estat de la granja i marca el flux de funcionament.

> Patró: igual que `CCODIS` a `exemplesTADS/`, exposa una funció `Farm_motor()` que es crida un cop per tic des del bucle principal.

---

## 1. Dades que ha de mantenir (variables internes)

```c
// Estat de la granja
static unsigned char inicialitzat;          // 0 = sense INITIALIZE rebut, 1 = funcionant
static char          nomGranja[N];          // "P2FB", etc.
static unsigned int  tempsAnimals[4];       // periode produccio de cada especie

// Animals (max 24 segons UI Java: 3 files x 8 cols)
#define MAX_ANIMALS 24
static struct {
    unsigned char especie;   // 0=PORC, 1=VACA, 2=CAVALL, 3=GALLINA
    unsigned char num;       // num dintre de l'especie
    unsigned char estat;     // 0=SLEEP, 1=AWAKE
    unsigned int  tics;      // comptador per al cicle son/desperta
} animals[MAX_ANIMALS];
static unsigned char nAnimals;

// Productes (els 4 que Java espera al histograma)
static unsigned int productes[4];   // 0=Llet, 1=Pernil, 2=Ous, 3=Pinzells

// Mode rebel·lio
static unsigned char rebellio;       // 0/1

// Cursor de seleccio (per al joystick)
static unsigned char animalSeleccionat;   // index dins animals[]

// Buffer de recepcio serie (linia acabada en \n)
static char  bufferSerie[64];
static unsigned char bufferLen;
```

---

## 2. Inicialització — `Farm_Init()`

Es crida **una vegada** al programa principal abans del bucle:

```
Farm_Init():
  1. Init dels altres TADs:
       Serial_Init()
       LcInit(2, 16)              // o la geometria que toqui
       Pols_Init()                 // joystick / polsadors
       AD_Init(canal)              // si hi ha potenciometre
       E2PROM_Init()               // si guarda estat persistent
       TI_NewTimer(...)            // un timer per al cicle son/desperta

  2. Estat inicial:
       inicialitzat = 0
       nAnimals     = 0
       rebellio     = 0
       Per i=0..3: productes[i] = 0
       bufferLen    = 0

  3. NO envia res per serie encara: espera que Java faci INITIALIZE.
```

---

## 3. Bucle de l'ordre principal (`main`)

```c
while(1) {
    if (TI_NewTic()) {           // sincronisme: una passada per tic
        Pols_motor();
        LcMotor();
        Farm_motor();            // <-- aquest TAD
    }
}
```

`Farm_motor` és l'únic que té lògica d'aplicacio. Els altres motors són hardware.

---

## 4. `Farm_motor()` — què comprova cada tic, i en quin ordre

```
Farm_motor():
  ┌──────────────────────────────────────────────────┐
  │ PAS 1: llegir serie entrant                      │
  └──────────────────────────────────────────────────┘
  while (Serial_CharAvail()):
      c = Serial_GetChar()
      if (c == '\n'):
          processaComanda(bufferSerie)   // veure §5
          bufferLen = 0
      else if (bufferLen < BUFFER_MAX):
          bufferSerie[bufferLen++] = c

  ┌──────────────────────────────────────────────────┐
  │ PAS 2: si encara no hem rebut INITIALIZE, sortir │
  └──────────────────────────────────────────────────┘
  if (!inicialitzat) return

  ┌──────────────────────────────────────────────────┐
  │ PAS 3: avançar temporitzadors d'animals          │
  └──────────────────────────────────────────────────┘
  Per cada animal a:
      a.tics++
      si a.tics >= tempsAnimals[a.especie]:
          a.tics = 0
          si a.estat == AWAKE:
              productes[a.especie]++       // genera producte
              // (en rebel·lio, comportament alterat — veure §8)
          // L'animal mai canvia per si sol de SLEEP a AWAKE:
          // nomes el desperta el SLEEP_SUCCESSFUL del Java o el RESET

  ┌──────────────────────────────────────────────────┐
  │ PAS 4: gestionar joystick (callbacks de POLS)    │
  └──────────────────────────────────────────────────┘
  // POLS ja crida RBx_Pols(1) quan detecta un flanc.
  // El TAD_FARM consulta unes flags pròpies que ha posat
  // en aquells callbacks (similar a CCODIS.flagPols)
  if (flagAmunt):    Serial_SendStr("MOVE_UP\r\n");    flagAmunt=0
  if (flagAvall):    Serial_SendStr("MOVE_DOWN\r\n");  flagAvall=0
  if (flagEsq):      Serial_SendStr("MOVE_LEFT\r\n");  flagEsq=0
  if (flagDreta):    Serial_SendStr("MOVE_RIGHT\r\n"); flagDreta=0
  if (flagSelect):   Serial_SendStr("SELECT\r\n");     flagSelect=0
  // Java rep aquests events i ja s'encarrega de moure el cursor a la GUI
  // i decidir quin SLEEP/CONSUME/etc enviar.

  ┌──────────────────────────────────────────────────┐
  │ PAS 5: refrescar LCD (segons l'animal seleccionat)│
  └──────────────────────────────────────────────────┘
  if (canviSeleccio || canviEstat):
      if (!LcIsBusy()):
          LcClear()
          LcGotoXY(0,0); LcPutString(nomEspecie(animals[sel].especie))
          LcGotoXY(0,1); LcPutString(animals[sel].estat? "AWAKE" : "SLEEP")
          canviSeleccio = 0
```

---

## 5. `processaComanda(s)` — comandes que arriben de Java

Cada `case` mira el prefix de `s` i actua. **No respon res si encara no arriba el `\n`**.

```
processaComanda(s):

  ┌────────────────────────────────────────────────────────────┐
  │ "INITIALIZE:nom$t1$t2$t3$t4"                              │
  └────────────────────────────────────────────────────────────┘
   Pre: inicialitzat == 0 (si ja ho era, RESET implicit)
   - Parsejar el nom i els 4 temps (separador '$')
   - Guardar a nomGranja[] i tempsAnimals[]
   - Crear animals predeterminats (segons especificacio)
   - Posar inicialitzat = 1
   - Iniciar comptadors de productes a 0
   - (Opcional) confirmar amb un envia de DATA_ANIMALS + FINISH

  ┌────────────────────────────────────────────────────────────┐
  │ "GET_ANIMALS"                                              │
  └────────────────────────────────────────────────────────────┘
   - Per cada animal:
       Serial_SendStr("DATA_ANIMALS:")
       Serial_SendStr(nomEspecie(a.especie))
       Serial_SendStr("$")
       Serial_SendInt(a.num)
       Serial_SendStr("$")
       Serial_SendStr(a.estat ? "AWAKE" : "SLEEP")
       Serial_SendStr("\r\n")
   - Al final: Serial_SendStr("FINISH\r\n")
   NOTA: l'enviament tambe ha de ser cooperatiu (cua de bytes a TAD_SERIAL)
         per no bloquejar el motor durant 24 línies.

  ┌────────────────────────────────────────────────────────────┐
  │ "GET_PRODUCTS"                                             │
  └────────────────────────────────────────────────────────────┘
   - Serial_SendStr("DATA_PRODUCTS:")
     <Llet>$<Pernil>$<Ous>$<Pinzells>\r\n

  ┌────────────────────────────────────────────────────────────┐
  │ "RESET"                                                    │
  └────────────────────────────────────────────────────────────┘
   - Tornar tots els animals a estat per defecte (segons enunciat)
   - Productes a 0
   - rebellio = 0
   - Mantenir inicialitzat = 1 (no caldra tornar a fer INITIALIZE)
   - LCD: Clear

  ┌────────────────────────────────────────────────────────────┐
  │ "CONSUME:n"   (n = 0..3)                                   │
  └────────────────────────────────────────────────────────────┘
   - Si productes[n] > 0: productes[n]--
   - (Sense resposta: Java ja ho ha registrat localment)

  ┌────────────────────────────────────────────────────────────┐
  │ "SLEEP:especie$num"                                        │
  └────────────────────────────────────────────────────────────┘
   - Buscar l'animal amb aquesta especie + num
   - Comprovar precondicions:
       * existeix
       * a.estat == SLEEP        (nomes te sentit despertar-lo)
       * (qualsevol altra restriccio del enunciat: temps minim, etc.)
   - Si OK:
       a.estat = AWAKE
       a.tics  = 0
       Serial_SendStr("SLEEP_SUCCESSFUL\r\n")
   - Si NO OK:
       Serial_SendStr("SLEEP_UNSUCCESSFUL\r\n")

  ┌────────────────────────────────────────────────────────────┐
  │ "START_REBELLION" / "STOP_REBELLION"                       │
  └────────────────────────────────────────────────────────────┘
   - rebellio = 1 / 0
   - (Possibles efectes: parar produccio, encendre LED, ...)

  ┌────────────────────────────────────────────────────────────┐
  │ Default                                                    │
  └────────────────────────────────────────────────────────────┘
   - Comanda desconeguda: ignorar (o respondre amb error)
```

---

## 6. Cicle de vida d'un animal

```
                    ┌────────┐
   INITIALIZE  ───▶ │ SLEEP  │ ──────────────┐
                    └────────┘               │
                        ▲                    │
                        │  RESET             │ SLEEP:nom$num
                        │                    │ (Java demana despertar)
                        │                    ▼
                        │                ┌────────┐
                        └────────────────│ AWAKE  │
                          (cicle complet)└────────┘
                                          │
                                          │ cada tempsAnimals[especie] tics:
                                          │   productes[especie]++
                                          │   (i possiblement torna a SLEEP
                                          │    segons enunciat)
                                          ▼
```

**Punts a comprovar a cada transicio:**
- SLEEP → AWAKE: només per missatge SLEEP del Java (no automatic)
- AWAKE → SLEEP: després de produir N productes, o per timeout, segons l'enunciat
- En `RESET`: tots a SLEEP, `tics = 0`

---

## 7. Mode rebel·lió

L'enunciat (PDF) defineix els detalls exactes. Esquemàticament:

- `rebellio == 1` modifica algun comportament: per exemple, els animals AWAKE no produeixen, o produeixen un producte diferent, o l'LCD parpalleja, o sona un buzzer.
- El TAD_FARM nomes ha d'**afegir un `if (rebellio) ...`** als llocs adequats, sense estats nous complexos.

---

## 8. Tres regles d'or per no trencar el sistema cooperatiu

1. **Mai bloquegis dins de Farm_motor**. Tot el que tardi més d'un tic ha d'estar repartit en passades successives (igual que LCD i el seu motor).
2. **Mai cridis Lcxxx() sense comprovar `LcIsBusy()`**, o usa la cua del TAD_LCD com a coixí (que ja fa fins a 3).
3. **L'enviament de cadenes llargues per serie** (com `GET_ANIMALS` que pot ser 24 línies) també necessita cua. Si `TAD_SERIAL` no la té, has de fer un mini-estat dins del Farm_motor que envii una línia per tic.

---

## 9. Diagrama compacte del que passa cada tic

```
TIC
 │
 ├──▶ POLS_motor (debouncing dels polsadors)
 │
 ├──▶ LCD_motor  (avanca operacio LCD pendent)
 │
 └──▶ FARM_motor
        │
        ├─ 1) Drenar bytes del UART; si \n → processaComanda(linia)
        ├─ 2) Si no inicialitzat → return
        ├─ 3) Avançar tics dels animals; produir productes
        ├─ 4) Empenyer events del joystick cap a Java per serie
        └─ 5) Refrescar l'LCD si la seleccio o l'estat ha canviat
```

---

## 10. Resum del contracte amb la part Java

| Java envia | TAD_FARM ha de | Resposta |
|---|---|---|
| `INITIALIZE:nom$t1$t2$t3$t4` | crear granja, posar a SLEEP, productes=0 | (cap, o `DATA_ANIMALS:...`+`FINISH`) |
| `GET_ANIMALS` | recórrer la llista | n × `DATA_ANIMALS:nom$num$estat` + `FINISH` |
| `GET_PRODUCTS` | empaquetar comptadors | `DATA_PRODUCTS:n1$n2$n3$n4` |
| `RESET` | tornar a estat inicial | (cap) |
| `CONSUME:n` | decrementar `productes[n]` | (cap) |
| `SLEEP:nom$num` | validar i despertar | `SLEEP_SUCCESSFUL` o `SLEEP_UNSUCCESSFUL` |
| `START_REBELLION` | activar mode | (cap) |
| `STOP_REBELLION` | desactivar mode | (cap) |

| TAD_FARM envia (espontàniament) | Quan |
|---|---|
| `MOVE_UP/DOWN/LEFT/RIGHT` | flanc del polsador corresponent |
| `SELECT` | flanc del polsador select |
