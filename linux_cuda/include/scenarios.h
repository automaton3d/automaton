/*
 * scenarios.h
 */

#ifndef SCENARIOS_H_
#define SCENARIOS_H_

#include "jpeg.h"
#include "brick.h"

#define HEL	1
#define ELT	2
#define CHI	4
#define GRV	8
#define CLR	16

extern boolean showGrid, showBox, showAxes;
extern Brick *d_pri0;

/// Functions ///

void BurstScenario();
void VacuumScenario();
void NoUXUScenario();
void UScenario();
void UXUScenario();
void GRAVScenario();
void SEEDScenario();
void BigBangScenario();
void LonePScenario();

#endif /* SCENARIOS_H_ */

