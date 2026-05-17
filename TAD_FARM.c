#include <xc.h>
#include "pic18f4321.h"
#include "TAD_TIMER.H"
#include "TAD_SIOFARM.H"
#include "TAD_SIO.H"
#include "TAD_LCD.H"
#include "TAD_ADC.H"
#include "TAD_EEPROM.H"
#include "TAD_LED.H"
#include "TAD_FARM.H"

// Especies: 0 vaca, 1 porc, 2 cavall, 3 gallina.
// Estat animal: 0 awake, 1 dormit.
// Limits fixos: 24 animals, 15 per especie, minim 3 per especie, 4 especies.

#define UN_SEGON 2400
#define SON 120
#define T_DORMIR 5
#define MEMORIA_GUARDADA 67
#define INICI_GUARDAR_ANIMALS 2
#define MIDA_ANIMAL_EEPROM 8
#define FI_CARREGA_EEPROM 255

#define ESPAI "\b \b"
#define PRODUCTES "P:"
#define ANIMALS "A:"
#define LINIA_NOVA "\r\n"
#define FINISH "F\r\n"
#define SLEEP_OK "Y\r\n"
#define SLEEP_NO "N\r\n"

static const unsigned char tempsProducte[4] = {47, 31, 23, 13};
// Posicions pel numero al LCD.
static const unsigned char colNumeroProducte[4] = {6, 8, 9, 5};
static const unsigned char colNumeroAnimal[4] = {6, 6, 8, 9};
static const char *textProducte[4] = {"Llet: ", "Pernil: ", "Pinzell: ", "Ous: "};
static const char *textAnimal[4] = {"Vaca: ", "Porc: ", "Cavall: ", "Gallina: "};
static const char *textTipusJava[4] = {"VACA", "PORC", "CAVALL", "GALLINA"};

static char bufferData[14 + 1];
static char *liniaJava;
static char dataLCD[11];
static char nomLCD[17];
static char numeroJava[4];

static unsigned char indexBufferData;
static unsigned char overflowBufferData;
static unsigned char indexJava;

static unsigned char idxSortidaJava;
static unsigned char enviantJava;
static unsigned char animalSortidaJava;
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
static unsigned char cuaAvisInfo[3];
static unsigned char quantsAvis;
static unsigned char rebellio;
static unsigned char resetPas;
static unsigned char pendentsMinim;

static unsigned char dia, mes, hora, minut, segon;

static unsigned char tipusAnimal[24];
static unsigned char numAnimal[24];
static unsigned char sonAnimal[24];
static unsigned char tempsDespert[24];
static unsigned char totalAnimals;
static unsigned char Especies[4];
static unsigned char despertsEspecie[4];

static unsigned char tempsGeneracio[4];
static unsigned char segonsGeneracio[4];
static unsigned char segonsProducte[4];
static unsigned char productes[4];

static unsigned char enviantAnimals;
static unsigned char idxAnimalEnviar;
static unsigned char sleepTipus;
static unsigned char sleepNumero;
static unsigned char sleepIndex;
static unsigned char esperaLdr;
static unsigned char idxRevisaSon;
static unsigned char revisaSon;
static unsigned char idxCarregaEeprom;
static unsigned char adrecaRevisaSon;
static unsigned char adrecaCarregaEeprom;
static unsigned char adrecaEepromAnimal;
static unsigned char auxTipus;
static unsigned char diaUltimDescans;
static unsigned char mesUltimDescans;
static unsigned char horaUltimDescans;
static unsigned char minutUltimDescans;


static void NetejaGranja(void) {
    estat = 0; // demanar data
    indexBufferData = 0;
    overflowBufferData = 0;
    idxSortidaJava = 0;
    enviantJava = 0;
    missatgeData = 0;
    idxMissatgeData = 0;
    enviantAnimals = 0;
    idxAnimalEnviar = 0;
    esperaLdr = 0;
    hiHaAvisLcd = 0;
    quantsAvis = 0;
    estatAvis = 0;
    avisProducte = 0;
    avisTipus = 0;
    avisValor = 0;
    nomLCD[0] = 0;
    dataLCD[0] = 0;
    rebellio = 0;
    resetPas = 0;
    idxRevisaSon = 0;
    revisaSon = 0;
    estatDespresLcd = 5; // esperar java
    dia = mes = hora = minut = segon = 0;
    totalAnimals = 0;
    pendentsMinim = 4 * 3;
    tempsGeneracio[0] = tempsGeneracio[1] = tempsGeneracio[2] = tempsGeneracio[3] = 0;
    segonsGeneracio[0] = segonsGeneracio[1] = segonsGeneracio[2] = segonsGeneracio[3] = 0;
    segonsProducte[0] = segonsProducte[1] = segonsProducte[2] = segonsProducte[3] = 0;
    Especies[0] = Especies[1] = Especies[2] = Especies[3] = 0;
    productes[0] = productes[1] = productes[2] = productes[3] = 0;
    despertsEspecie[0] = despertsEspecie[1] = despertsEspecie[2] = despertsEspecie[3] = 0;
}

static void IniciaMissatgeData(unsigned char missatge) {
    missatgeData = missatge;
    idxMissatgeData = 0;
}

static void GuardaAnimal(unsigned char index) {
    adrecaEepromAnimal = index;
    adrecaEepromAnimal *= MIDA_ANIMAL_EEPROM;
    adrecaEepromAnimal += INICI_GUARDAR_ANIMALS;
    EEPROM_Escriu(adrecaEepromAnimal, tipusAnimal[index]);
    EEPROM_Escriu(adrecaEepromAnimal + 1, numAnimal[index]);
    EEPROM_Escriu(adrecaEepromAnimal + 2, sonAnimal[index]);
    EEPROM_Escriu(adrecaEepromAnimal + 3, dia);
    EEPROM_Escriu(adrecaEepromAnimal + 4, mes);
    EEPROM_Escriu(adrecaEepromAnimal + 5, hora);
    EEPROM_Escriu(adrecaEepromAnimal + 6, minut);
    EEPROM_Escriu(adrecaEepromAnimal + 7, segon);
    EEPROM_Escriu(1, totalAnimals);
}

static void ActualitzaSonPerData(void) {
    despertsEspecie[0] = despertsEspecie[1] = despertsEspecie[2] = despertsEspecie[3] = 0;
    idxRevisaSon = 0;
    adrecaRevisaSon = INICI_GUARDAR_ANIMALS;
    revisaSon = 2;
}

static void ComencaCarregaEeprom(void) {
    totalAnimals = 0;
    idxCarregaEeprom = FI_CARREGA_EEPROM;
    pendentsMinim = 4 * 3;

    if(EEPROM_Llegeix(0) != MEMORIA_GUARDADA) return;

    totalAnimals = EEPROM_Llegeix(1);
    if(totalAnimals > 24) {
        totalAnimals = 0;
        return;
    }

    if(totalAnimals != 0) {
        idxCarregaEeprom = 0;
        adrecaCarregaEeprom = INICI_GUARDAR_ANIMALS;
    }
}

static void CarregaUnAnimalEeprom(void) {
    adrecaEepromAnimal = adrecaCarregaEeprom;
    tipusAnimal[idxCarregaEeprom] = EEPROM_Llegeix(adrecaEepromAnimal);
    numAnimal[idxCarregaEeprom] = EEPROM_Llegeix(adrecaEepromAnimal + 1);
    sonAnimal[idxCarregaEeprom] = EEPROM_Llegeix(adrecaEepromAnimal + 2);
    auxTipus = tipusAnimal[idxCarregaEeprom];
    if(auxTipus >= 4) auxTipus = 0;

    tipusAnimal[idxCarregaEeprom] = auxTipus;
    tempsDespert[idxCarregaEeprom] = 0;
    if(Especies[auxTipus] < 3) pendentsMinim--;
    Especies[auxTipus]++;
    if(sonAnimal[idxCarregaEeprom] == 0) despertsEspecie[auxTipus]++;

    idxCarregaEeprom++;
    adrecaCarregaEeprom += MIDA_ANIMAL_EEPROM;
    if(idxCarregaEeprom >= totalAnimals) idxCarregaEeprom = FI_CARREGA_EEPROM;
}

void FARM_Init(void) {
    NetejaGranja();
    TI_NewTimer(&timerFarm);
    TI_NewTimer(&timerLcd);
    TI_NewTimer(&timerSleep);
    TI_ResetTics(timerFarm);
    TI_ResetTics(timerLcd);
    TI_ResetTics(timerSleep);
    ComencaCarregaEeprom();
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
    if(indexBufferData != 14) return 0;

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

static void NumeroAText(unsigned char numero) {
    unsigned char centenes;
    unsigned char desenes;
    unsigned char indexNumero;

    // Evitem / i % perque XC8 no fa cabre les rutines de divisio.
    centenes = 0;
    desenes = 0;
    indexNumero = 0;

    if(numero >= 100) {
        numero -= 100;
        centenes++;
        if(numero >= 100) {
            numero -= 100;
            centenes++;
        }
        numeroJava[indexNumero] = centenes + '0';
        indexNumero++;
    }

    if(numero >= 80) {
        numero -= 80;
        desenes += 8;
    }
    if(numero >= 40) {
        numero -= 40;
        desenes += 4;
    }
    if(numero >= 20) {
        numero -= 20;
        desenes += 2;
    }
    if(numero >= 10) {
        numero -= 10;
        desenes++;
    }

    if(centenes != 0 || desenes != 0) {
        numeroJava[indexNumero] = desenes + '0';
        indexNumero++;
    }

    numeroJava[indexNumero] = numero + '0';
    indexNumero++;
    numeroJava[indexNumero] = 0;
}

static void EnviaTextJava(const char *text) {
    if(SIO_PutString(text)) idxSortidaJava++;
}

static void IniciaSortidaJava(unsigned char tipusSortida) {
    idxSortidaJava = 0;
    enviantJava = tipusSortida;
}

static void IniciaTextJava(const char *text) {
    textJava = text;
    IniciaSortidaJava(1);
}

static void EnviaNumeroJava(unsigned char num) {
    NumeroAText(num);
    EnviaTextJava(numeroJava);
}

static void MotorSortidaJava(void) {
    if(enviantJava == 1) {
        if(SIO_PutString(textJava)) enviantJava = 0;
    } else if(enviantJava == 2) {
        switch(idxSortidaJava) {
            case 0: EnviaTextJava(PRODUCTES); break;
            case 1: EnviaNumeroJava(productes[0]); break;
            case 2: EnviaTextJava("$"); break;
            case 3: EnviaNumeroJava(productes[1]); break;
            case 4: EnviaTextJava("$"); break;
            case 5: EnviaNumeroJava(productes[3]); break;
            case 6: EnviaTextJava("$"); break;
            case 7: EnviaNumeroJava(productes[2]); break;
            case 8: EnviaTextJava(LINIA_NOVA); break;
            default: enviantJava = 0; break;
        }
    } else if(enviantJava == 3) {
        switch(idxSortidaJava) {
            case 0: EnviaTextJava(ANIMALS); break;
            case 1: EnviaTextJava(textTipusJava[tipusAnimal[animalSortidaJava]]); break;
            case 2: EnviaTextJava("$"); break;
            case 3: EnviaNumeroJava(numAnimal[animalSortidaJava]); break;
            case 4: EnviaTextJava("$"); break;
            case 5: EnviaTextJava((sonAnimal[animalSortidaJava] == 1) ? "SLEEP" : "AWAKE"); break;
            case 6: EnviaTextJava(LINIA_NOVA); break;
            default: enviantJava = 0; break;
        }
    }
}

static void MotorMissatgeData(void) {
    const char *text;

    if(missatgeData == 0) return;

    if(missatgeData == 3) {
        text = ESPAI;
    } else if(missatgeData == 1) {
        text = "\n\rDate and time correct\n\r";
    } else {
        text = "\n\rPlease input a correct date\n\r";
    }

    if(text[idxMissatgeData] == 0) {
        if(SIOFARM_TxBuida()) {
            IniciaMissatgeData(0);
        }
        return;
    }

    if(SIOFARM_EnviaCaracter(text[idxMissatgeData])) {
        idxMissatgeData++;
    }
}

static void AfegeixAvis(unsigned char avis) {
    if(quantsAvis >= 3) return;

    cuaAvisInfo[quantsAvis] = avis;
    quantsAvis++;
}

static void AvisAnimal(unsigned char tipus) {
    AfegeixAvis(tipus);
}

static void AvisProducte(unsigned char tipus) {
    AfegeixAvis(tipus | 0x04);
}

static void MotorAvisLcd(void) {
    if(hiHaAvisLcd) return;

    if(estatAvis == 0) {
        if(quantsAvis == 0) return;

        avisTipus = cuaAvisInfo[0];
        quantsAvis--;
        if(quantsAvis > 0) cuaAvisInfo[0] = cuaAvisInfo[1];
        if(quantsAvis > 1) cuaAvisInfo[1] = cuaAvisInfo[2];
        avisProducte = avisTipus & 0x04;
        avisTipus &= 0x03;
        if(avisProducte) {
            avisValor = productes[avisTipus];
            avisColNumero = colNumeroProducte[avisTipus];
        } else {
            avisValor = Especies[avisTipus];
            avisColNumero = colNumeroAnimal[avisTipus];
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
                    LcPutString("Nou Producte");
                } else {
                    LcPutString("Nou Animal");
                }
                estatAvis = 3;
            }
            break;

        case 3:
            if(!LcIsBusy()) {
                LcGotoXY(0, 1);
                if(avisProducte) {
                    LcPutString(textProducte[avisTipus]);
                } else {
                    LcPutString(textAnimal[avisTipus]);
                }
                estatAvis = 4;
            }
            break;

        case 4:
            if(!LcIsBusy()) {
                NumeroAText(avisValor);
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
    if(totalAnimals >= 24) return;
    if(Especies[tipus] >= 15) return;
    if(Especies[tipus] >= 3 && (24 - totalAnimals) <= pendentsMinim) return;

    tipusAnimal[totalAnimals] = tipus;
    if(Especies[tipus] == 0) segonsProducte[tipus] = 0;
    if(Especies[tipus] < 3) pendentsMinim--;
    Especies[tipus]++;
    numAnimal[totalAnimals] = Especies[tipus];
    sonAnimal[totalAnimals] = 0;
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
        segonsGeneracio[tipus]++;
        if(segonsGeneracio[tipus] >= tempsGeneracio[tipus]) {
            segonsGeneracio[tipus] = 0;
            AfegeixAnimal(tipus);
        }
    }
}

static void ActualitzaProducte(unsigned char tipus) {
    segonsProducte[tipus]++;
    if(segonsProducte[tipus] >= tempsProducte[tipus]) {
        segonsProducte[tipus] = 0;
        Produeix(tipus);
    }
}

static void RevisaSonAnimal(void) {
    if(!revisaSon) return;
    if(idxRevisaSon >= totalAnimals) {
        revisaSon = 0;
        return;
    }

    if(revisaSon == 2) {
        adrecaEepromAnimal = adrecaRevisaSon;
        diaUltimDescans = EEPROM_Llegeix(adrecaEepromAnimal + 3);
        mesUltimDescans = EEPROM_Llegeix(adrecaEepromAnimal + 4);
        horaUltimDescans = EEPROM_Llegeix(adrecaEepromAnimal + 5);
        minutUltimDescans = EEPROM_Llegeix(adrecaEepromAnimal + 6);

        if(diaUltimDescans != dia || mesUltimDescans != mes || hora > horaUltimDescans || (hora == horaUltimDescans && minut >= minutUltimDescans + 2)) {
            sonAnimal[idxRevisaSon] = 1;
            tempsDespert[idxRevisaSon] = SON;
        } else {
            sonAnimal[idxRevisaSon] = 0;
            tempsDespert[idxRevisaSon] = 0;
            despertsEspecie[tipusAnimal[idxRevisaSon]]++;
        }
        idxRevisaSon++;
        adrecaRevisaSon += MIDA_ANIMAL_EEPROM;
        return;
    }

    if(sonAnimal[idxRevisaSon] == 0) {
        if(tempsDespert[idxRevisaSon] < SON) {
            tempsDespert[idxRevisaSon]++;
        } else {
            sonAnimal[idxRevisaSon] = 1;
            auxTipus = tipusAnimal[idxRevisaSon];
            if(despertsEspecie[auxTipus] != 0) despertsEspecie[auxTipus]--;
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
        estatDespresLcd = 6; // funcionament
        estat = 2; // lcd clear
    }

    if(!rebellio) {
        ActualitzaProducte(0);
        ActualitzaProducte(1);
        ActualitzaProducte(2);
        ActualitzaProducte(3);
    }

    GeneraEspecie(0);
    GeneraEspecie(1);
    GeneraEspecie(2);
    GeneraEspecie(3);

    idxRevisaSon = 0;
    revisaSon = 1;
}

static void ResetGranja(void) {
    NetejaGranja();
    resetPas = 1;
}

static void ProcessaConsum(void) {
    switch(liniaJava[2]) {
        case '0': // fried egg: 1 ou
            if(productes[3] != 0) productes[3]--;
            break;

        case '1': // omelette with ham: 1 ou i 1 pernil
            if(productes[3] != 0 && productes[1] != 0) {
                productes[3]--;
                productes[1]--;
            }
            break;

        case '2': // cacaolat: 2 llets
            if(productes[0] >= 2) productes[0] -= 2;
            break;

        case '3': // painting: 2 pinzells
            if(productes[2] >= 2) productes[2] -= 2;
            break;
    }

    IniciaSortidaJava(2);
}

static unsigned char NumDespres(void) {
    unsigned char num;

    num = liniaJava[indexJava] - '0';
    indexJava++;

    if(liniaJava[indexJava] > '/') {
        num = num * 10 + liniaJava[indexJava] - '0';
        indexJava++;

        if(liniaJava[indexJava] > '/') {
            num = num * 10 + liniaJava[indexJava] - '0';
            indexJava++;
        }
    }

    if(liniaJava[indexJava] == '$') indexJava++;

    return num;
}

static void ProcessaInitialize(void) {
    unsigned char indexMissatge;
    unsigned char indexNom;

    indexMissatge = 2;
    indexNom = 0;
    for(; liniaJava[indexMissatge] != '$' && liniaJava[indexMissatge] != 0 && indexNom < 15; indexMissatge++) {
        nomLCD[indexNom] = liniaJava[indexMissatge];
        indexNom++;
    }
    nomLCD[indexNom] = '!';
    nomLCD[indexNom + 1] = 0;

    if(liniaJava[indexMissatge] == '$') indexMissatge++;
    indexJava = indexMissatge;
    tempsGeneracio[0] = NumDespres();
    tempsGeneracio[1] = NumDespres();
    tempsGeneracio[2] = NumDespres();
    tempsGeneracio[3] = NumDespres();

    segonsGeneracio[0] = segonsGeneracio[1] = segonsGeneracio[2] = segonsGeneracio[3] = 0;
    segonsProducte[0] = segonsProducte[1] = segonsProducte[2] = segonsProducte[3] = 0;
    EEPROM_Escriu(0, MEMORIA_GUARDADA);
    if(dia != 0) {
        TI_ResetTics(timerFarm);
        estatDespresLcd = 6; // funcionament
        estat = 2; // lcd clear
    } else {
        estat = 1; // llegir data
    }
}

static void PreparaAnimal(void) {
    animalSortidaJava = idxAnimalEnviar;
    IniciaSortidaJava(3);
}

static void EnviaFinish(void) {
    IniciaTextJava(FINISH);
}

static void ProcessaSleep(void) {
    unsigned char indexNumero;

    indexNumero = 6;
    if(liniaJava[2] == 'P') {
        sleepTipus = 1;
    } else if(liniaJava[2] == 'C') {
        sleepTipus = 2;
        indexNumero = 8;
    } else if(liniaJava[2] == 'G') {
        sleepTipus = 3;
        indexNumero = 9;
    } else {
        sleepTipus = 0;
    }
    if(liniaJava[indexNumero] == '$') indexNumero++;
    indexJava = indexNumero;
    sleepNumero = NumDespres();
    sleepIndex = 0;
    esperaLdr = 2;
}

static void ProcessaJava(void) {
    if(liniaJava[0] != 'I' && estat != 6) return; // funcionament

    switch(liniaJava[0]) {
        case 'P':
            IniciaSortidaJava(2);
            break;

        case 'A':
            enviantAnimals = 1;
            idxAnimalEnviar = 0;
            break;

        case 'R':
            ResetGranja();
            break;

        case 'B':
            if(liniaJava[1] == '1') {
                rebellio = 1;
            } else {
                rebellio = 0;
                segonsProducte[0] = segonsProducte[1] = segonsProducte[2] = segonsProducte[3] = 0;
            }
            break;

        case 'C':
            ProcessaConsum();
            break;

        case 'S':
            ProcessaSleep();
            break;

        case 'I':
            ProcessaInitialize();
            break;
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

    c = SIOFARM_LlegeixCaracter();
    if(c == 0) return;

    if(c == '\r') {
        bufferData[indexBufferData] = 0;

        if(!overflowBufferData && ValidaData()) {
            IniciaMissatgeData(1);
            ActualitzaSonPerData();
            if(nomLCD[0] != 0) {
                if(estat != 6) TI_ResetTics(timerFarm);
                estatDespresLcd = 6; // funcionament
            } else {
                estatDespresLcd = 5; // esperar java
            }
            estat = 2; // lcd clear
        } else {
            IniciaMissatgeData(2);
            if(inicial) estat = 0; // demanar data
        }
        indexBufferData = 0;
        overflowBufferData = 0;
    } else if(c == 8 || c == 127) {
        if(indexBufferData != 0) {
            indexBufferData--;
            overflowBufferData = 0;
            IniciaMissatgeData(3);
        }
    } else if(c >= 32 && c <= 126) {
        SIOFARM_EnviaCaracter(c);

        if(indexBufferData < 14) {
            bufferData[indexBufferData] = c;
            indexBufferData++;
        } else {
            overflowBufferData = 1;
        }
    }
}

void FARM_Motor(void) {
    unsigned int ticsFarm;

    ticsFarm = TI_GetTics(timerFarm);

    // Control temps
    if(ticsFarm >= UN_SEGON) {
        TI_ResetTics(timerFarm);
        if(estat == 6) NouSegon(); // funcionament
    }

    LED_Actiu(!rebellio && dia != 0 && nomLCD[0] != 0);

    // Carrega EEPROM
    if(idxCarregaEeprom != FI_CARREGA_EEPROM) {
        CarregaUnAnimalEeprom();
        return;
    }

    // Avisos LCD
    if(hiHaAvisLcd && TI_GetTics(timerLcd) >= UN_SEGON * 3) {
        hiHaAvisLcd = 0;
        if(quantsAvis == 0 && (estat == 6 || estat == 5)) { // funcionament o esperar java
            estatDespresLcd = estat;
            estat = 2; // lcd clear
        }
    }

    MotorMissatgeData();
    if(missatgeData != 0) return;

    MotorAvisLcd();
    LlegeixJava();

    // Tasques pendents
    if(estatAvis != 0) return;

    if(revisaSon) {
        RevisaSonAnimal();
        return;
    }

    if(resetPas != 0) {
        EEPROM_Escriu(1, 0);
        estatDespresLcd = 5; // esperar java
        estat = 2; // lcd clear
        resetPas = 0;
        return;
    }

    MotorSortidaJava();
    if(enviantJava) return;

    // Enviament animals
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

    // Control dormir
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
            if(sonAnimal[sleepIndex] == 1) despertsEspecie[tipusAnimal[sleepIndex]]++;
            sonAnimal[sleepIndex] = 0;
            tempsDespert[sleepIndex] = 0;
            GuardaAnimal(sleepIndex);
            IniciaTextJava(SLEEP_OK);
            esperaLdr = 0;
        } else if(TI_GetTics(timerSleep) >= UN_SEGON * T_DORMIR) {
            IniciaTextJava(SLEEP_NO);
            esperaLdr = 0;
        }
        return;
    }

    // Estat principal
    switch(estat) {
        case 0: // demanar data
            indexBufferData = 0;
            overflowBufferData = 0;
            estat = 1; // llegir data
            break;

        case 1: // llegir data
            LlegeixDataTerminal(1);
            break;

        case 2: // lcd clear
            if(!LcIsBusy()) {
                LcClear();
                estat = 3; // lcd titol
            }
            break;

        case 3: // lcd titol
            if(!LcIsBusy()) {
                LcGotoXY(0, 0);
                LcPutString(nomLCD);
                estat = 4; // lcd data
            }
            break;

        case 4: // lcd data
            if(!LcIsBusy()) {
                LcGotoXY(0, 1);
                LcPutString(dataLCD);
                estat = estatDespresLcd;
            }
            break;

        case 5: // esperar java
            LlegeixDataTerminal(0);
            break;

        case 6: // funcionament
            LlegeixDataTerminal(0);
            break;
    }
}
