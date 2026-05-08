#ifndef TAD_FARM_H
#define TAD_FARM_H

/*
 * TAD_FARM
 *
 * Controller central de la practica LSFarm.
 * Gestiona la maquina d'estats global del sistema:
 *
 *   1) Demanar data/hora per SIOFARM (terminal)
 *   2) Esperar inicialitzacio de Java (nom granja + temps generacio)
 *   3) Funcionament normal
 *   4) Rebellio
 */

void FARM_Init(void);
void FARM_Motor(void);

#endif