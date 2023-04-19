/*
 * combine.h
 *
 *  Created on: 24 de mar. de 2023
 *      Author: Alexandre
 */

#ifndef COMBINE_H_
#define COMBINE_H_

#define N         (6*32768)
//#define N         (6*16384)
//#define N         (6*8192)
//#define N         (6*4096)
//#define N         (6*264329)
#define C_MASK    0x07
#define W0_MASK   0x08
#define W1_MASK   0x10
#define Q_MASK    0x20

#define REPEAT    1.0

void init();
void orbis();
void dark();
int tryQuark();
int tryElec();
int tryQuark_D();
int tryElec_D();
int tryZB();
int tryZB_D();

#endif /* COMBINE_H_ */
