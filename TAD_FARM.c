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

#define UN_SEGON 2400
#define SON 120
#define T_DORMIR 5
#define MEMORIA_GUARDADA 67
#define INICI_GUARDAR_ANIMALS 2
#define MIDA_ANIMAL_EEPROM 8
#define FI_CARREGA_EEPROM 255

//temps de produccio de cada proudcte: 0 llet, 1 pernil, 2 pinzell, 3 ou.
static const unsigned char tProduccio[4] = {47, 31, 23, 13};
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

static unsigned char iSortidaJava;
static unsigned char enviantJava;
static unsigned char animalSortidaJava;
static const char *textJava;

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

static unsigned char dia;
static unsigned char mes;
static unsigned char hora;
static unsigned char minut;
static unsigned char segon;

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
static unsigned char iAnimalEnviar;

static unsigned char sleepTipus;
static unsigned char sleepNumero;
static unsigned char iDormit;

static unsigned char esperaLdr;
static unsigned char iRevisaSon;

static unsigned char revisaSon;

static unsigned char iCarregaEeprom;
static unsigned char iGuardaEeprom;
static unsigned char campGuardaEeprom;
static unsigned char fiGuardaEeprom;
static unsigned char guardarCapcalera;

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
    iSortidaJava = 0;
    enviantJava = 0;

    enviantAnimals = 0;
    iAnimalEnviar = 0;

    esperaLdr = 0;
    hiHaAvisLcd = 0;
    quantsAvis = 0;
    estatAvis = 0;
    avisProducte = 0;
    avisTipus = 0;
    avisValor = 0;

    //nom granja i data.
    nomLCD[0] = 0;
    dataLCD[0] = 0;

    rebellio = 0;
    resetPas = 0;

    iRevisaSon = 0;
    revisaSon = 0;
    iGuardaEeprom = 0;
    campGuardaEeprom = 8;
    fiGuardaEeprom = 0;
    guardarCapcalera = 0;
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

static void GuardaCapcaleraEeprom(void) {
    guardarCapcalera = 1;
}

static void GuardaAnimalEeprom(unsigned char animal) {
    if(campGuardaEeprom >= 8) {
        iGuardaEeprom = animal;
        fiGuardaEeprom = animal + 1;
        campGuardaEeprom = 0;
    } else {
        if(animal < iGuardaEeprom) iGuardaEeprom = animal;
        if(animal >= fiGuardaEeprom) fiGuardaEeprom = animal + 1;
    }
}

static unsigned char MotorGuardaEeprom(void) {
    unsigned char offset;
    unsigned char valor;

    if(guardarCapcalera == 0 && campGuardaEeprom >= 8) return 0;
    if(EEPROM_Busy()) return 1;

    if(guardarCapcalera == 1) {
        EEPROM_Escriu(0, MEMORIA_GUARDADA);
        guardarCapcalera++;
        return 1;
    }

    if(guardarCapcalera == 2) {
        EEPROM_Escriu(1, totalAnimals);
        guardarCapcalera = 0;
        return 1;
    }

    offset = iGuardaEeprom;
    offset <<= 3;
    offset += campGuardaEeprom;
    adrecaEepromAnimal = INICI_GUARDAR_ANIMALS + offset;

    switch(campGuardaEeprom) {
        case 0: valor = tipusAnimal[iGuardaEeprom]; break;
        case 1: valor = numAnimal[iGuardaEeprom]; break;
        case 2: valor = sonAnimal[iGuardaEeprom]; break;
        case 3: valor = dia; break;
        case 4: valor = mes; break;
        case 5: valor = hora; break;
        case 6: valor = minut; break;
        default: valor = segon; break;
    }

    EEPROM_Escriu(adrecaEepromAnimal, valor);
    campGuardaEeprom++;
    if(campGuardaEeprom >= 8) {
        iGuardaEeprom++;
        if(iGuardaEeprom < fiGuardaEeprom) campGuardaEeprom = 0;
    }

    return 1;
}

static void ActualitzaSonPerData(void){
    despertsEspecie[0] = despertsEspecie[1] = despertsEspecie[2] = despertsEspecie[3] = 0;

    iRevisaSon = 0;
    adrecaRevisaSon = INICI_GUARDAR_ANIMALS;
    revisaSon = 2;

}

static void ComencaCarregaEeprom(void){

    totalAnimals = 0;
    iCarregaEeprom = FI_CARREGA_EEPROM;

    pendentsMinim = 4 * 3;//12

    if(EEPROM_Llegeix(0) != MEMORIA_GUARDADA) return;

    totalAnimals = EEPROM_Llegeix(1);
    if(totalAnimals > 24) {

        totalAnimals = 0;
        return;

    }

    if(totalAnimals != 0) {

        iCarregaEeprom = 0;
        adrecaCarregaEeprom = INICI_GUARDAR_ANIMALS;
    }
}

static void CarregaUnAnimalEeprom(void) {

    adrecaEepromAnimal = adrecaCarregaEeprom;

    tipusAnimal[iCarregaEeprom] = EEPROM_Llegeix(adrecaEepromAnimal);
    numAnimal[iCarregaEeprom] = EEPROM_Llegeix(adrecaEepromAnimal + 1);
    sonAnimal[iCarregaEeprom] = EEPROM_Llegeix(adrecaEepromAnimal + 2);
    auxTipus = tipusAnimal[iCarregaEeprom];

    if(auxTipus >= 4) {
        auxTipus = 0;
    }

    tipusAnimal[iCarregaEeprom] = auxTipus;
    tempsDespert[iCarregaEeprom] = 0;
    if(Especies[auxTipus] < 3) pendentsMinim--;

    Especies[auxTipus]++;

    if(sonAnimal[iCarregaEeprom] == 0) despertsEspecie[auxTipus]++;

    iCarregaEeprom++;
    adrecaCarregaEeprom += MIDA_ANIMAL_EEPROM;

    if(iCarregaEeprom >= totalAnimals) iCarregaEeprom = FI_CARREGA_EEPROM;
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
    // si arriba fins aqui es que la data te el format pertinent

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

    if(numero >= 20){

        numero -= 20;
        desenes += 2;
    }
    if(numero >= 10){
        numero -= 10;
        desenes++;

    }

    if(centenes != 0 || desenes != 0){
        numeroJava[indexNumero] = desenes + '0';
        indexNumero++;
    }

    numeroJava[indexNumero] = numero + '0';
    indexNumero++;
    numeroJava[indexNumero] = 0;
}

static void EnviaTextJava(const char *text){
    if(SIO_PutString(text)) iSortidaJava++;
}

static void IniciaSortidaJava(unsigned char tipusSortida){


    iSortidaJava = 0;
    enviantJava = tipusSortida;
}

static void IniciaTextJava(const char *text){

    textJava = text;
    IniciaSortidaJava(1);
}

static void EnviaNumeroJava(unsigned char num){

    NumeroAText(num);
    EnviaTextJava(numeroJava);
}

static void MotorSortidaJava(void){

    if(enviantJava == 1) {
        if(SIO_PutString(textJava)) enviantJava = 0;
    } else if(enviantJava == 2){

        switch(iSortidaJava) {
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
    } else if(enviantJava == 3){

        switch(iSortidaJava) {
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

static void AfegeixAvis(unsigned char avis){
    if(quantsAvis >= 3) return;

    cuaAvisInfo[quantsAvis] = avis;
    quantsAvis++;
}

static void MotorAvisLcd(void){


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
    GuardaCapcaleraEeprom();
    GuardaAnimalEeprom(totalAnimals - 1);
    AfegeixAvis(tipus);
}

static void Produeix(unsigned char tipus) {
    if(despertsEspecie[tipus] != 0) {
        productes[tipus] += despertsEspecie[tipus];
        AfegeixAvis(tipus | 0x04);
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
    if(segonsProducte[tipus] >= tProduccio[tipus]) {
        segonsProducte[tipus] = 0;
        Produeix(tipus);
    }
}

static void RevisaSonAnimal(void) {
    if(!revisaSon) return;
    if(iRevisaSon >= totalAnimals) {
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
            sonAnimal[iRevisaSon] = 1;
            tempsDespert[iRevisaSon] = SON;
        } else {
            sonAnimal[iRevisaSon] = 0;
            tempsDespert[iRevisaSon] = 0;
            despertsEspecie[tipusAnimal[iRevisaSon]]++;
        }
        iRevisaSon++;
        adrecaRevisaSon += MIDA_ANIMAL_EEPROM;
        return;
    }

    if(sonAnimal[iRevisaSon] == 0) {
        if(tempsDespert[iRevisaSon] < SON) {
            tempsDespert[iRevisaSon]++;
        } else {
            sonAnimal[iRevisaSon] = 1;
            auxTipus = tipusAnimal[iRevisaSon];
            if(despertsEspecie[auxTipus] != 0) despertsEspecie[auxTipus]--;
        }
    }
    iRevisaSon++;
}

// aquesta funcio gestiona el pas del temps de la granja respecte els nostres tics.
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
    if(hora >= 24){

        hora = 0;
        dia++;

        if(dia > 31){

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

                if(dataLCD[4] > '9'){
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

    iRevisaSon = 0;
    revisaSon = 1;
}

static void ProcessaConsum(void) {

    switch(liniaJava[2]) {
        case '0': // ou fregit: 1 ou

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

        case '3': // pinzels: 2 pinzells

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

static void ProcessaInitialize(void){

    unsigned char indexMissatge;
    unsigned char indexNom;

    indexMissatge = 2;
    indexNom = 0;
    for(; liniaJava[indexMissatge] != '$' && liniaJava[indexMissatge] != 0 && indexNom < 15; indexMissatge++){

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
    GuardaCapcaleraEeprom();
    if(dia != 0) {
        TI_ResetTics(timerFarm);
        estatDespresLcd = 6; // funcionament
        estat = 2; // lcd clear
    } else {
        estat = 1; // llegir data
    }
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
    iDormit = 0;
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
            iAnimalEnviar = 0;
            break;

        case 'R':
            NetejaGranja();
            resetPas = 1;
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
            SIOFARM_IniciaMissatge(MISSATGE_DATA_CORRECTA);
            ActualitzaSonPerData();
            if(nomLCD[0] != 0) {
                if(estat != 6) TI_ResetTics(timerFarm);
                estatDespresLcd = 6; // funcionament
            } else {
                estatDespresLcd = 5; // esperar java
            }
            estat = 2; // lcd clear
        } else {
            SIOFARM_IniciaMissatge(MISSATGE_DATA_INCORRECTA);
            if(inicial) estat = 0; // demanar data
        }
        indexBufferData = 0;
        overflowBufferData = 0;
    } else if(c == 8 || c == 127) {
        if(indexBufferData != 0) {
            indexBufferData--;
            overflowBufferData = 0;
            SIOFARM_IniciaMissatge(MISSATGE_BORRAR);
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
    if(iCarregaEeprom != FI_CARREGA_EEPROM) {
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

    MotorAvisLcd();
    LlegeixJava();

    // Tasques pendents
    if(estatAvis != 0) return;

    if(revisaSon) {
        RevisaSonAnimal();
        return;
    }

    if(resetPas == 1) {
        GuardaCapcaleraEeprom();
        resetPas = 2;
        return;
    }

    if(resetPas == 2) {
        estatDespresLcd = 5; // esperar java
        estat = 2; // lcd clear
        resetPas = 0;
        return;
    }

    MotorSortidaJava();
    if(enviantJava) return;

    // Enviament animals
    if(enviantAnimals) {
        if(iAnimalEnviar < totalAnimals) {
            animalSortidaJava = iAnimalEnviar;
            IniciaSortidaJava(3);
            iAnimalEnviar++;
        } else {
            IniciaTextJava(FINISH);
            enviantAnimals = 0;
        }
        return;
    }

    // Control dormir
    if(esperaLdr) {
        if(esperaLdr == 2) {
            if(iDormit >= totalAnimals) {
                esperaLdr = 0;
            } else if(tipusAnimal[iDormit] == sleepTipus && numAnimal[iDormit] == sleepNumero) {
                esperaLdr = 1;
                TI_ResetTics(timerSleep);
            } else {
                iDormit++;
            }
            return;
        }

        if(AD_GetMostra(2) < 60) {
            if(sonAnimal[iDormit] == 1) despertsEspecie[tipusAnimal[iDormit]]++;
            sonAnimal[iDormit] = 0;
            tempsDespert[iDormit] = 0;
            GuardaAnimalEeprom(iDormit);
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

    MotorGuardaEeprom();
}
