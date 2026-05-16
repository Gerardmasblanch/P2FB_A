#include <xc.h>
#include "pic18f4321.h"
#include "TAD_TIMER.H"
#include "TAD_SIOFARM.H"
#include "TAD_SIO.H"
#include "TAD_LCD.H"
#include "TAD_ADC.H"
#include "TAD_EEPROM.H"
#include "TAD_FARM.H"

#define ESTAT_DEMANAR_DATA 0
#define ESTAT_LLEGIR_DATA 1
#define ESTAT_LCD_CLEAR 2
#define ESTAT_LCD_TITOL 3
#define ESTAT_LCD_DATA 4
#define ESTAT_ESPERAR_JAVA 5
#define ESTAT_FUNCIONAMENT 6

#define LONG_DATA 14
#define LONG_JAVA 48
#define MAX_ANIMALS 24
#define MAX_ANIMALS_ESPECIE 15
#define MIN_ANIMALS_ESPECIE 3
#define NUM_ESPECIES 4
#define MIDA_AVIS 4

#define VACA 0
#define PORC 1
#define CAVALL 2
#define GALLINA 3

#define AWAKE 0
#define DORMIT 1

#define TICS_SEGON 2400
#define TEMPS_SON 120
#define TEMPS_DESCANS 5
#define TEMPS_LLET 47
#define TEMPS_PERNIL 31
#define TEMPS_PINZELL 23
#define TEMPS_OUS 13
#define MSG_DATA_OK 1
#define MSG_DATA_ERROR 2
#define MSG_BACKSPACE 3
#define RECORD_SHIFT 3
#define EEPROM_FI 255
#define ADRECA_REGISTRE(index) ((unsigned char)(EEPROM_RECORDS + ((index) << RECORD_SHIFT)))

#define TXT_LLET "Llet: "
#define TXT_PERNIL "Pernil: "
#define TXT_PINZELL "Pinzell: "
#define TXT_OUS "Ous: "
#define TXT_VACA "Vaca: "
#define TXT_PORC "Porc: "
#define TXT_CAVALL "Cavall: "
#define TXT_GALLINA "Gallina: "
#define TXT_BACKSPACE "\b \b"
#define TXT_DATA_OK "\n\rDate and time correct\n\r"
#define TXT_DATA_ERROR "\n\rPlease input a correct date\n\r"
#define TXT_TIPUS_VACA "VACA"
#define TXT_TIPUS_PORC "PORC"
#define TXT_TIPUS_CAVALL "CAVALL"
#define TXT_TIPUS_GALLINA "GALLINA"
#define TXT_NOU_PRODUCTE "Nou Producte"
#define TXT_NOU_ANIMAL "Nou Animal"
#define TXT_DATA_PRODUCTS "DATA_PRODUCTS:%V$%P$%G$%C\r\n"
#define TXT_DATA_ANIMALS "DATA_ANIMALS:%T$%N$%S\r\n"
#define TXT_SLEEP "SLEEP"
#define TXT_AWAKE "AWAKE"
#define TXT_FINISH "FINISH\r\n"
#define TXT_SLEEP_OK "SLEEP_SUCCESSFUL\r\n"
#define TXT_SLEEP_KO "SLEEP_UNSUCCESSFUL\r\n"

static const unsigned char tempsProducte[NUM_ESPECIES] = {TEMPS_LLET, TEMPS_PERNIL, TEMPS_PINZELL, TEMPS_OUS};
static const unsigned char colProducte[NUM_ESPECIES] = {6, 8, 9, 5};
static const unsigned char colAnimal[NUM_ESPECIES] = {6, 6, 8, 9};
static const char *textProducte[NUM_ESPECIES] = {TXT_LLET, TXT_PERNIL, TXT_PINZELL, TXT_OUS};
static const char *textAnimal[NUM_ESPECIES] = {TXT_VACA, TXT_PORC, TXT_CAVALL, TXT_GALLINA};
static const char *textTipusJava[NUM_ESPECIES] = {TXT_TIPUS_VACA, TXT_TIPUS_PORC, TXT_TIPUS_CAVALL, TXT_TIPUS_GALLINA};

static char bufferData[LONG_DATA + 1];
static char *liniaJava;
static char dataLCD[11];
static char nomLCD[17];
static char numeroJava[4];

static unsigned char idxBuffer;
static unsigned char overflowBuffer;

static unsigned char idxSortidaJava;
static unsigned char enviantJava;
static unsigned char idxTextJava;
static unsigned char idxNumeroJava;
static unsigned char midaNumeroJava;
static unsigned char animalSortidaJava;
static const char *formatJava;
static const char *textJava;
static unsigned char missatgeData;
static unsigned char idxMissatgeData;

static unsigned char estat;
static unsigned char timerFarm;
static unsigned char timerLcd;
static unsigned char timerSleep;
static unsigned char hiHaAvisLcd;
static unsigned char estatDespresLcd;
static unsigned char avisProducte;
static unsigned char avisTipus;
static unsigned char estatAvis;
static unsigned char avisValor;
static unsigned char avisColNumero;
static unsigned char cuaAvisInfo[MIDA_AVIS];
static unsigned char quantsAvis;
static unsigned char rebellio;
static unsigned char resetPas;
static unsigned char ledBaixant;
static unsigned char pendentsMinim;

static unsigned char dia, mes, hora, minut, segon;

static unsigned char tipusAnimal[MAX_ANIMALS];
static unsigned char numAnimal[MAX_ANIMALS];
static unsigned char sonAnimal[MAX_ANIMALS];
static unsigned char tempsDespert[MAX_ANIMALS];
static unsigned char totalAnimals;
static unsigned char quantsEspecie[NUM_ESPECIES];
static unsigned char despertsEspecie[NUM_ESPECIES];

static unsigned char tempsGeneracio[NUM_ESPECIES];
static unsigned char comptGeneracio[NUM_ESPECIES];
static unsigned char comptProducte[NUM_ESPECIES];
static unsigned char productes[NUM_ESPECIES];

static unsigned char enviantAnimals;
static unsigned char idxAnimalEnviar;
static unsigned char sleepTipus;
static unsigned char sleepNumero;
static unsigned char sleepIndex;
static unsigned char esperaLdr;
static unsigned char idxRevisaSon;
static unsigned char revisaSon;
static unsigned char idxCarregaEeprom;

static void EnviaProductes(void);

static void GuardaAnimal(unsigned char index) {
    unsigned char adreca;

    adreca = ADRECA_REGISTRE(index);
    EEPROM_Write(adreca, tipusAnimal[index]);
    EEPROM_Write(adreca + 1, numAnimal[index]);
    EEPROM_Write(adreca + 2, sonAnimal[index]);
    EEPROM_Write(adreca + 3, dia);
    EEPROM_Write(adreca + 4, mes);
    EEPROM_Write(adreca + 5, hora);
    EEPROM_Write(adreca + 6, minut);
    EEPROM_Write(adreca + 7, segon);
    EEPROM_Write(1, totalAnimals);
}

static void ActualitzaSonPerData(void) {
    despertsEspecie[0] = despertsEspecie[1] = despertsEspecie[2] = despertsEspecie[3] = 0;
    idxRevisaSon = 0;
    revisaSon = 2;
}

static void ComencaCarregaEeprom(void) {
    totalAnimals = 0;
    idxCarregaEeprom = EEPROM_FI;
    pendentsMinim = NUM_ESPECIES * MIN_ANIMALS_ESPECIE;
    quantsEspecie[0] = quantsEspecie[1] = quantsEspecie[2] = quantsEspecie[3] = 0;
    despertsEspecie[0] = despertsEspecie[1] = despertsEspecie[2] = despertsEspecie[3] = 0;
    productes[0] = productes[1] = productes[2] = productes[3] = 0;

    if(EEPROM_Read(0) != EEPROM_MAGIC) return;

    totalAnimals = EEPROM_Read(1);
    if(totalAnimals > MAX_ANIMALS) {
        totalAnimals = 0;
        return;
    }

    if(totalAnimals != 0) idxCarregaEeprom = 0;
}

static void PosaDataLCD(void) {
    dataLCD[0] = bufferData[0];
    dataLCD[1] = bufferData[1];
    dataLCD[2] = '/';
    dataLCD[3] = bufferData[3];
    dataLCD[4] = bufferData[4];
    dataLCD[5] = '/';
    dataLCD[6] = '2';
    dataLCD[7] = '0';
    dataLCD[8] = '2';
    dataLCD[9] = '6';
    dataLCD[10] = 0;
}

static unsigned char ValidaData(void) {
    if(idxBuffer != LONG_DATA) return 0;

    if(bufferData[2] != '/') return 0;
    if(bufferData[5] != ' ') return 0;
    if(bufferData[8] != ':') return 0;
    if(bufferData[11] != ':') return 0;

    dia = (bufferData[0] - '0') * 10 + bufferData[1] - '0';
    mes = (bufferData[3] - '0') * 10 + bufferData[4] - '0';
    hora = (bufferData[6] - '0') * 10 + bufferData[7] - '0';
    minut = (bufferData[9] - '0') * 10 + bufferData[10] - '0';
    segon = (bufferData[12] - '0') * 10 + bufferData[13] - '0';

    if(dia < 1 || dia > 31) return 0;
    if(mes < 1 || mes > 12) return 0;
    if(hora > 23) return 0;
    if(minut > 59) return 0;
    if(segon > 59) return 0;

    PosaDataLCD();
    return 1;
}

static void ComencaSortidaJava(const char *text) {
    formatJava = text;
    textJava = 0;
    idxSortidaJava = 0;
    idxTextJava = 0;
    idxNumeroJava = 0;
    midaNumeroJava = 0;
    enviantJava = 1;
}

static void PreparaNumeroJava(unsigned char num) {
    unsigned char centenes;
    unsigned char desenes;

    centenes = 0;
    desenes = 0;
    idxNumeroJava = 0;
    midaNumeroJava = 0;

    if(num >= 100) {
        num -= 100;
        centenes++;
        if(num >= 100) {
            num -= 100;
            centenes++;
        }
        numeroJava[midaNumeroJava] = (char)(centenes + '0');
        midaNumeroJava++;
    }

    if(num >= 80) {
        num -= 80;
        desenes += 8;
    }
    if(num >= 40) {
        num -= 40;
        desenes += 4;
    }
    if(num >= 20) {
        num -= 20;
        desenes += 2;
    }
    if(num >= 10) {
        num -= 10;
        desenes++;
    }

    if(centenes != 0 || desenes != 0) {
        numeroJava[midaNumeroJava] = (char)(desenes + '0');
        midaNumeroJava++;
    }

    numeroJava[midaNumeroJava] = (char)(num + '0');
    midaNumeroJava++;
    numeroJava[midaNumeroJava] = 0;
}

static void PreparaTokenJava(char token) {
    switch(token) {
        case 'V':
            PreparaNumeroJava(productes[VACA]);
            break;
        case 'P':
            PreparaNumeroJava(productes[PORC]);
            break;
        case 'G':
            PreparaNumeroJava(productes[GALLINA]);
            break;
        case 'C':
            PreparaNumeroJava(productes[CAVALL]);
            break;
        case 'N':
            PreparaNumeroJava(numAnimal[animalSortidaJava]);
            break;
        case 'T':
            textJava = textTipusJava[tipusAnimal[animalSortidaJava]];
            idxTextJava = 0;
            break;
        case 'S':
            textJava = (sonAnimal[animalSortidaJava] == DORMIT) ? TXT_SLEEP : TXT_AWAKE;
            idxTextJava = 0;
            break;
    }
}

static void MotorSortidaJava(void) {
    char c;

    if(!enviantJava) return;

    if(textJava != 0) {
        c = textJava[idxTextJava];
        if(c == 0) {
            textJava = 0;
            idxTextJava = 0;
        } else if(SIO_PutChar(c)) {
            idxTextJava++;
        }
        return;
    }

    if(midaNumeroJava != 0) {
        if(SIO_PutChar(numeroJava[idxNumeroJava])) {
            idxNumeroJava++;
            if(idxNumeroJava >= midaNumeroJava) midaNumeroJava = 0;
        }
        return;
    }

    c = formatJava[idxSortidaJava];
    if(c == 0) {
        enviantJava = 0;
        idxSortidaJava = 0;
        return;
    }

    if(c == '%') {
        idxSortidaJava++;
        PreparaTokenJava(formatJava[idxSortidaJava]);
        idxSortidaJava++;
    } else if(SIO_PutChar(c)) {
        idxSortidaJava++;
    }
}

static void MotorMissatgeData(void) {
    const char *text;

    if(missatgeData == 0) return;

    if(missatgeData == MSG_BACKSPACE) {
        text = TXT_BACKSPACE;
    } else if(missatgeData == MSG_DATA_OK) {
        text = TXT_DATA_OK;
    } else {
        text = TXT_DATA_ERROR;
    }

    if(text[idxMissatgeData] == 0) {
        if(SIOFARM_TxLliure()) {
            missatgeData = 0;
            idxMissatgeData = 0;
        }
        return;
    }

    if(SIOFARM_EnviaCaracter(text[idxMissatgeData])) {
        idxMissatgeData++;
    }
}

static void AvisAnimal(unsigned char tipus) {
    if(quantsAvis >= MIDA_AVIS) return;

    cuaAvisInfo[quantsAvis] = tipus;
    quantsAvis++;
}

static void AvisProducte(unsigned char tipus) {
    if(quantsAvis >= MIDA_AVIS) return;

    cuaAvisInfo[quantsAvis] = tipus | 0x04;
    quantsAvis++;
}

static void MotorAvisLcd(void) {
    if(hiHaAvisLcd) return;

    if(estatAvis == 0) {
        if(quantsAvis == 0) return;

        avisTipus = cuaAvisInfo[0];
        quantsAvis--;
        if(quantsAvis > 0) cuaAvisInfo[0] = cuaAvisInfo[1];
        if(quantsAvis > 1) cuaAvisInfo[1] = cuaAvisInfo[2];
        if(quantsAvis > 2) cuaAvisInfo[2] = cuaAvisInfo[3];
        avisProducte = avisTipus & 0x04;
        avisTipus &= 0x03;
        if(avisProducte) {
            avisValor = productes[avisTipus];
            avisColNumero = colProducte[avisTipus];
        } else {
            avisValor = quantsEspecie[avisTipus];
            avisColNumero = colAnimal[avisTipus];
        }
        estatAvis = 1;
    }

    switch(estatAvis) {
        case 1:
            if(!LcIsBusy()) {
                LcClear();
                estatAvis = 2;
            }
            break;

        case 2:
            if(!LcIsBusy()) {
                LcGotoXY(0, 0);
                if(avisProducte) {
                    LcPutString(TXT_NOU_PRODUCTE);
                } else {
                    LcPutString(TXT_NOU_ANIMAL);
                }
                estatAvis = 3;
            }
            break;

        case 3:
            if(!LcIsBusy()) {
                LcGotoXY(0, 1);
                if(avisProducte) {
                    LcPutString((char *)textProducte[avisTipus]);
                } else {
                    LcPutString((char *)textAnimal[avisTipus]);
                }
                estatAvis = 4;
            }
            break;

        case 4:
            if(!LcIsBusy()) {
                PreparaNumeroJava(avisValor);
                LcGotoXY(avisColNumero, 1);
                estatAvis = 5;
            }
            break;

        case 5:
            if(!LcIsBusy()) {
                LcPutString(numeroJava);
                TI_ResetTics(timerLcd);
                hiHaAvisLcd = 1;
                estatAvis = 0;
            }
            break;
    }
}

static void AfegeixAnimal(unsigned char tipus) {
    if(totalAnimals >= MAX_ANIMALS) return;
    if(quantsEspecie[tipus] >= MAX_ANIMALS_ESPECIE) return;
    if(quantsEspecie[tipus] >= MIN_ANIMALS_ESPECIE && (MAX_ANIMALS - totalAnimals) <= pendentsMinim) return;

    tipusAnimal[totalAnimals] = tipus;
    if(quantsEspecie[tipus] == 0) comptProducte[tipus] = 0;
    if(quantsEspecie[tipus] < MIN_ANIMALS_ESPECIE) pendentsMinim--;
    quantsEspecie[tipus]++;
    numAnimal[totalAnimals] = quantsEspecie[tipus];
    sonAnimal[totalAnimals] = AWAKE;
    tempsDespert[totalAnimals] = 0;
    despertsEspecie[tipus]++;
    totalAnimals++;
    GuardaAnimal(totalAnimals - 1);
    AvisAnimal(tipus);
}

static void Produeix(unsigned char tipus) {
    if(despertsEspecie[tipus] != 0) {
        productes[tipus] += despertsEspecie[tipus];
        AvisProducte(tipus);
    }
}

static void GeneraEspecie(unsigned char tipus) {
    if(tempsGeneracio[tipus] != 0) {
        comptGeneracio[tipus]++;
        if(comptGeneracio[tipus] >= tempsGeneracio[tipus]) {
            comptGeneracio[tipus] = 0;
            AfegeixAnimal(tipus);
        }
    }
}

static void ActualitzaProducte(unsigned char tipus) {
    comptProducte[tipus]++;
    if(comptProducte[tipus] >= tempsProducte[tipus]) {
        comptProducte[tipus] = 0;
        Produeix(tipus);
    }
}

static void RevisaSonAnimal(void) {
    unsigned char tipus;
    unsigned char adreca;
    unsigned char d;
    unsigned char m;
    unsigned char h;
    unsigned char mn;

    if(!revisaSon) return;
    if(idxRevisaSon >= totalAnimals) {
        revisaSon = 0;
        return;
    }

    if(revisaSon == 2) {
        adreca = ADRECA_REGISTRE(idxRevisaSon);
        d = EEPROM_Read(adreca + 3);
        m = EEPROM_Read(adreca + 4);
        h = EEPROM_Read(adreca + 5);
        mn = EEPROM_Read(adreca + 6);

        if(d != dia || m != mes || hora > h || (hora == h && minut >= mn + 2)) {
            sonAnimal[idxRevisaSon] = DORMIT;
            tempsDespert[idxRevisaSon] = TEMPS_SON;
        } else {
            sonAnimal[idxRevisaSon] = AWAKE;
            tempsDespert[idxRevisaSon] = 0;
            despertsEspecie[tipusAnimal[idxRevisaSon]]++;
        }
        idxRevisaSon++;
        return;
    }

    if(sonAnimal[idxRevisaSon] == AWAKE) {
        if(tempsDespert[idxRevisaSon] < TEMPS_SON) {
            tempsDespert[idxRevisaSon]++;
        } else {
            sonAnimal[idxRevisaSon] = DORMIT;
            tipus = tipusAnimal[idxRevisaSon];
            if(despertsEspecie[tipus] != 0) despertsEspecie[tipus]--;
        }
    }
    idxRevisaSon++;
}

static void NouSegon(void) {
    segon++;
    if(segon >= 60) {
        segon = 0;
        minut++;
    }
    if(minut >= 60) {
        minut = 0;
        hora++;
    }
    if(hora >= 24) {
        hora = 0;
        dia++;
        if(dia > 31) {
            dia = 1;
            dataLCD[0] = '0';
            dataLCD[1] = '1';
            mes++;
            if(mes > 12) {
                mes = 1;
                dataLCD[3] = '0';
                dataLCD[4] = '1';
            } else {
                dataLCD[4]++;
                if(dataLCD[4] > '9') {
                    dataLCD[4] = '0';
                    dataLCD[3]++;
                }
            }
        } else {
            dataLCD[1]++;
            if(dataLCD[1] > '9') {
                dataLCD[1] = '0';
                dataLCD[0]++;
            }
        }
        estatDespresLcd = ESTAT_FUNCIONAMENT;
        estat = ESTAT_LCD_CLEAR;
    }

    if(!rebellio) {
        ActualitzaProducte(VACA);
        ActualitzaProducte(PORC);
        ActualitzaProducte(CAVALL);
        ActualitzaProducte(GALLINA);
    }

    GeneraEspecie(VACA);
    GeneraEspecie(PORC);
    GeneraEspecie(CAVALL);
    GeneraEspecie(GALLINA);

    idxRevisaSon = 0;
    revisaSon = 1;
}

static void ResetGranja(void) {
    totalAnimals = 0;
    pendentsMinim = NUM_ESPECIES * MIN_ANIMALS_ESPECIE;
    enviantAnimals = 0;
    idxAnimalEnviar = 0;
    quantsEspecie[0] = quantsEspecie[1] = quantsEspecie[2] = quantsEspecie[3] = 0;
    productes[0] = productes[1] = productes[2] = productes[3] = 0;
    despertsEspecie[0] = despertsEspecie[1] = despertsEspecie[2] = despertsEspecie[3] = 0;
    comptGeneracio[0] = comptGeneracio[1] = comptGeneracio[2] = comptGeneracio[3] = 0;
    comptProducte[0] = comptProducte[1] = comptProducte[2] = comptProducte[3] = 0;
    rebellio = 0;
    LATAbits.LATA4 = 0;
    hiHaAvisLcd = 0;
    quantsAvis = 0;
    estatAvis = 0;
    nomLCD[0] = 0;
    dataLCD[0] = 0;
    resetPas = 1;
}

static void ActualitzaLed(unsigned int ticsFarm) {
    unsigned char intensitat;

    if(rebellio || dia == 0 || nomLCD[0] == 0) {
        LATAbits.LATA4 = 0;
        return;
    }

    if(!ledBaixant) {
        intensitat = (unsigned char)(ticsFarm >> 7);
    } else {
        intensitat = (unsigned char)((TICS_SEGON - ticsFarm) >> 7);
    }

    LATAbits.LATA4 = (((unsigned char)ticsFarm & 0x0F) < intensitat) ? 1 : 0;
}

static void ProcessaConsum(void) {
    switch(liniaJava[8]) {
        case '0': // fried egg: 1 ou
            if(productes[GALLINA] != 0) productes[GALLINA]--;
            break;

        case '1': // omelette with ham: 1 ou i 1 pernil
            if(productes[GALLINA] != 0 && productes[PORC] != 0) {
                productes[GALLINA]--;
                productes[PORC]--;
            }
            break;

        case '2': // cacaolat: 2 llets
            if(productes[VACA] >= 2) productes[VACA] -= 2;
            break;

        case '3': // painting: 2 pinzells
            if(productes[CAVALL] >= 2) productes[CAVALL] -= 2;
            break;
    }

    EnviaProductes();
}

static unsigned char NumDespres(unsigned char index) {
    unsigned char num;

    num = 0;
    for(; liniaJava[index] >= '0' && liniaJava[index] <= '9'; index++) {
        num = num * 10 + liniaJava[index] - '0';
    }

    return num;
}

static void ProcessaInitialize(void) {
    unsigned char i;
    unsigned char j;
    unsigned char camp;

    i = 11;
    j = 0;
    for(; liniaJava[i] != '$' && liniaJava[i] != 0 && j < 15; i++) {
        nomLCD[j] = liniaJava[i];
        j++;
    }
    nomLCD[j] = '!';
    nomLCD[j + 1] = 0;
    tempsGeneracio[0] = tempsGeneracio[1] = tempsGeneracio[2] = tempsGeneracio[3] = 0;

    camp = 0;
    for(; liniaJava[i] != 0 && camp < 4; i++) {
        if(liniaJava[i] == '$') {
            i++;
            tempsGeneracio[camp] = NumDespres(i);
            camp++;
        }
    }

    comptGeneracio[0] = comptGeneracio[1] = comptGeneracio[2] = comptGeneracio[3] = 0;
    comptProducte[0] = comptProducte[1] = comptProducte[2] = comptProducte[3] = 0;
    EEPROM_Write(0, EEPROM_MAGIC);
    if(dia != 0) {
        estatDespresLcd = ESTAT_FUNCIONAMENT;
        estat = ESTAT_LCD_CLEAR;
    } else {
        estat = ESTAT_LLEGIR_DATA;
    }
}

static void EnviaProductes(void) {
    ComencaSortidaJava(TXT_DATA_PRODUCTS);
}

static void PreparaAnimal(void) {
    animalSortidaJava = idxAnimalEnviar;
    ComencaSortidaJava(TXT_DATA_ANIMALS);
}

static void EnviaFinish(void) {
    ComencaSortidaJava(TXT_FINISH);
}

static void ProcessaSleep(void) {
    unsigned char i;

    i = 10;
    sleepTipus = VACA;
    if(liniaJava[6] == 'P') sleepTipus = PORC;
    if(liniaJava[6] == 'C') {
        sleepTipus = CAVALL;
        i = 12;
    }
    if(liniaJava[6] == 'G') {
        sleepTipus = GALLINA;
        i = 13;
    }
    if(liniaJava[i] == '$') i++;
    sleepNumero = NumDespres(i);
    sleepIndex = 0;
    esperaLdr = 2;
}

static void ProcessaJava(void) {
    if(liniaJava[0] == 'G' && liniaJava[4] == 'P') {
        EnviaProductes();
    } else if(liniaJava[0] == 'G') {
        enviantAnimals = 1;
        idxAnimalEnviar = 0;
    } else if(liniaJava[0] == 'R') {
        ResetGranja();
    } else if(liniaJava[0] == 'S' && liniaJava[1] == 'T') {
        if(liniaJava[2] == 'A') {
            rebellio = 1;
            LATAbits.LATA4 = 0;
        } else {
            rebellio = 0;
            comptProducte[0] = comptProducte[1] = comptProducte[2] = comptProducte[3] = 0;
        }
    } else if(liniaJava[0] == 'C') {
        ProcessaConsum();
    } else if(liniaJava[0] == 'S' && liniaJava[1] == 'L') {
        ProcessaSleep();
    } else if(liniaJava[0] == 'I') {
        ProcessaInitialize();
    }
}

static void LlegeixJava(void) {
    liniaJava = SIO_GetLine();
    if(liniaJava != 0) {
        ProcessaJava();
    }
}

static void LlegeixDataTerminal(unsigned char inicial) {
    char c;

    if(!SIOFARM_TxLliure()) return;
    c = SIOFARM_LlegeixCaracter();
    if(c == 0) return;

    if(c == '\r') {
        bufferData[idxBuffer] = 0;

        if(!overflowBuffer && ValidaData()) {
            missatgeData = MSG_DATA_OK;
            idxMissatgeData = 0;
            ActualitzaSonPerData();
            estatDespresLcd = estat;
            if(inicial || nomLCD[0] == 0) estatDespresLcd = ESTAT_ESPERAR_JAVA;
            if(nomLCD[0] != 0) estatDespresLcd = ESTAT_FUNCIONAMENT;
            estat = ESTAT_LCD_CLEAR;
        } else {
            missatgeData = MSG_DATA_ERROR;
            idxMissatgeData = 0;
            if(inicial) estat = ESTAT_DEMANAR_DATA;
        }
        idxBuffer = 0;
        overflowBuffer = 0;
    } else if(c == 8 || c == 127) {
        if(idxBuffer != 0) {
            idxBuffer--;
            overflowBuffer = 0;
            missatgeData = MSG_BACKSPACE;
            idxMissatgeData = 0;
        }
    } else if(c >= 32 && c <= 126) {
        SIOFARM_EnviaCaracter(c);

        if(idxBuffer < LONG_DATA) {
            bufferData[idxBuffer] = c;
            idxBuffer++;
        } else {
            overflowBuffer = 1;
        }
    }
}

void FARM_Init(void) {
    estat = ESTAT_DEMANAR_DATA;
    idxBuffer = 0;
    overflowBuffer = 0;
    idxSortidaJava = 0;
    enviantJava = 0;
    missatgeData = 0;
    idxMissatgeData = 0;
    enviantAnimals = 0;
    idxAnimalEnviar = 0;
    esperaLdr = 0;
    hiHaAvisLcd = 0;
    avisProducte = 0;
    avisTipus = 0;
    avisValor = 0;
    quantsAvis = 0;
    nomLCD[0] = 0;
    rebellio = 0;
    resetPas = 0;
    ledBaixant = 0;
    idxRevisaSon = 0;
    revisaSon = 0;
    estatAvis = 0;
    estatDespresLcd = ESTAT_ESPERAR_JAVA;
    dia = mes = hora = minut = segon = 0;
    tempsGeneracio[0] = tempsGeneracio[1] = tempsGeneracio[2] = tempsGeneracio[3] = 0;
    comptGeneracio[0] = comptGeneracio[1] = comptGeneracio[2] = comptGeneracio[3] = 0;
    comptProducte[0] = comptProducte[1] = comptProducte[2] = comptProducte[3] = 0;
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 0;
    TI_NewTimer(&timerFarm);
    TI_NewTimer(&timerLcd);
    TI_NewTimer(&timerSleep);
    TI_ResetTics(timerFarm);
    TI_ResetTics(timerLcd);
    TI_ResetTics(timerSleep);
    ComencaCarregaEeprom();
}

void FARM_Motor(void) {
    unsigned int ticsFarm;
    unsigned char adreca;
    unsigned char tipus;

    ticsFarm = TI_GetTics(timerFarm);

    if(ticsFarm >= TICS_SEGON) {
        TI_ResetTics(timerFarm);
        ledBaixant = !ledBaixant;
        if(estat == ESTAT_FUNCIONAMENT) NouSegon();
        ticsFarm = 0;
    }

    ActualitzaLed(ticsFarm);

    if(idxCarregaEeprom != EEPROM_FI) {
        adreca = ADRECA_REGISTRE(idxCarregaEeprom);
        tipusAnimal[idxCarregaEeprom] = EEPROM_Read(adreca);
        numAnimal[idxCarregaEeprom] = EEPROM_Read(adreca + 1);
        sonAnimal[idxCarregaEeprom] = EEPROM_Read(adreca + 2);
        tipus = tipusAnimal[idxCarregaEeprom];
        if(tipus >= NUM_ESPECIES) tipus = VACA;

        tipusAnimal[idxCarregaEeprom] = tipus;
        tempsDespert[idxCarregaEeprom] = 0;
        if(quantsEspecie[tipus] < MIN_ANIMALS_ESPECIE) pendentsMinim--;
        quantsEspecie[tipus]++;
        if(sonAnimal[idxCarregaEeprom] == AWAKE) despertsEspecie[tipus]++;

        idxCarregaEeprom++;
        if(idxCarregaEeprom >= totalAnimals) idxCarregaEeprom = EEPROM_FI;
        return;
    }

    if(hiHaAvisLcd && TI_GetTics(timerLcd) >= TICS_SEGON * 3) {
        hiHaAvisLcd = 0;
        if(quantsAvis == 0 && (estat == ESTAT_FUNCIONAMENT || estat == ESTAT_ESPERAR_JAVA)) {
            estatDespresLcd = estat;
            estat = ESTAT_LCD_CLEAR;
        }
    }

    MotorAvisLcd();
    MotorMissatgeData();
    LlegeixJava();

    if(missatgeData != 0) return;
    if(estatAvis != 0) return;

    if(revisaSon) {
        RevisaSonAnimal();
        return;
    }

    if(resetPas != 0) {
        EEPROM_Write(1, 0);
        estatDespresLcd = ESTAT_ESPERAR_JAVA;
        estat = ESTAT_LCD_CLEAR;
        resetPas = 0;
        return;
    }

    MotorSortidaJava();
    if(enviantJava) return;

    if(enviantAnimals) {
        if(idxAnimalEnviar < totalAnimals) {
            PreparaAnimal();
            idxAnimalEnviar++;
        } else {
            EnviaFinish();
            enviantAnimals = 0;
        }
        return;
    }

    if(esperaLdr) {
        if(esperaLdr == 2) {
            if(sleepIndex >= totalAnimals) {
                esperaLdr = 0;
            } else if(tipusAnimal[sleepIndex] == sleepTipus && numAnimal[sleepIndex] == sleepNumero) {
                esperaLdr = 1;
                TI_ResetTics(timerSleep);
            } else {
                sleepIndex++;
            }
            return;
        }

        if(AD_GetMostra(2) < 40) {
            if(sonAnimal[sleepIndex] == DORMIT) despertsEspecie[tipusAnimal[sleepIndex]]++;
            sonAnimal[sleepIndex] = AWAKE;
            tempsDespert[sleepIndex] = 0;
            GuardaAnimal(sleepIndex);
            ComencaSortidaJava(TXT_SLEEP_OK);
            esperaLdr = 0;
        } else if(TI_GetTics(timerSleep) >= TICS_SEGON * TEMPS_DESCANS) {
            ComencaSortidaJava(TXT_SLEEP_KO);
            esperaLdr = 0;
        }
        return;
    }

    switch(estat) {
        case ESTAT_DEMANAR_DATA:
            idxBuffer = 0;
            overflowBuffer = 0;
            estat = ESTAT_LLEGIR_DATA;
            break;

        case ESTAT_LLEGIR_DATA:
            LlegeixDataTerminal(1);
            break;

        case ESTAT_LCD_CLEAR:
            if(!LcIsBusy()) {
                LcClear();
                estat = ESTAT_LCD_TITOL;
            }
            break;

        case ESTAT_LCD_TITOL:
            if(!LcIsBusy()) {
                LcGotoXY(0, 0);
                LcPutString(nomLCD);
                estat = ESTAT_LCD_DATA;
            }
            break;

        case ESTAT_LCD_DATA:
            if(!LcIsBusy()) {
                LcGotoXY(0, 1);
                LcPutString(dataLCD);
                estat = estatDespresLcd;
            }
            break;

        case ESTAT_ESPERAR_JAVA:
            LlegeixDataTerminal(0);
            break;

        case ESTAT_FUNCIONAMENT:
            LlegeixDataTerminal(0);
            break;
    }
}
