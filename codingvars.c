/*************************************************************************\
 * JPEG-LS Lossless image encoder/decoder                                *
 * Copyright (C) 2010 Davide Bardone <davide.bardone@gmail.com>          *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
\*************************************************************************/

#include "codingvars.h"
uint32 min(uint32,uint32);
uint32 maxx(uint32,uint32);

static inline uint32 min(uint32 a,uint32 b){ return (a > b) ? b : a; }
static inline uint32 maxx(uint32 a,uint32 b){ return (a > b) ? a : b; }

static uint8 J_values[] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,5,5,6,6,7,7,8,9,10,11,12,13,14,15};

void init_codingvars(codingvars* vars, parameters params)
{
	/* A.2 Initializations and conventions */

	/* A.2.1 Initializations */

	uint16 i;
	uint32 A_init_value;

	(*vars).RUNindex = 0;
	(*vars).J = J_values;
	(*vars).prevRa = 0;
	(*vars).MAX_C=127;
	(*vars).MIN_C=-128;
	(*vars).BASIC_T1 = 3;
	(*vars).BASIC_T2 = 7;
	(*vars).BASIC_T3 = 21;

	(*vars).prevRam0 = 0;

	(*vars).PxNO = 0;
	(*vars).Errval_temp1 = 0;
	(*vars).Errval_temp2 = 0;
	(*vars).Errval_temp3 = 0;

	(*vars).temp = 0;
	(*vars).mv = 2;
	(*vars).nv = 2;
	(*vars).m0 = 0;
	(*vars).n0 = 0;

	(*vars).RANGE = floor( (float64)(params.MAXVAL + 2*params.NEAR) / (2*params.NEAR + 1) ) + 1;
	(*vars).qbpp = ceil( log2((*vars).RANGE) );
	(*vars).bpp = maxx( 2, log2(params.MAXVAL + 1) );
	(*vars).LIMIT = 2*( (*vars).bpp + maxx(8,(*vars).bpp) );

	A_init_value = maxx( 2, floor( (float64)((*vars).RANGE + 32)/64 ) );

	for(i=0; i<CONTEXTS; i++)
	{
		(*vars).A[i] = A_init_value;
		(*vars).N[i] = 1;
		(*vars).B[i] = 0;
		(*vars).C[i] = 0;
	}
	(*vars).A[CONTEXTS] = A_init_value;
	(*vars).A[CONTEXTS+1] = A_init_value;
	(*vars).N[CONTEXTS] = 1;
	(*vars).N[CONTEXTS+1] = 1;
	(*vars).Nn[0] = 0;
	(*vars).Nn[1] = 0;
}


void update_codingvars(codingvars* vars, parameters params)
{
	/* A.6.1 Update */

	(*vars).B[(*vars).Q] += (*vars).Errval*(2*params.NEAR + 1);
	(*vars).A[(*vars).Q] += abs((*vars).Errval);
	if((*vars).N[(*vars).Q] == params.RESET)
	{
		(*vars).A[(*vars).Q] = (*vars).A[(*vars).Q]>>1;
		if((*vars).B[(*vars).Q]>=0)
			(*vars).B[(*vars).Q] = (*vars).B[(*vars).Q]>>1;
		else
			(*vars).B[(*vars).Q] = -((1-(*vars).B[(*vars).Q])>>1);
		(*vars).N[(*vars).Q] = (*vars).N[(*vars).Q]>>1;
	}
	(*vars).N[(*vars).Q] += 1;

	/* A.6.2 Bias computation */

	if((*vars).B[(*vars).Q] <= -(*vars).N[(*vars).Q])
	{
		(*vars).B[(*vars).Q] += (*vars).N[(*vars).Q];
		if((*vars).C[(*vars).Q] > (*vars).MIN_C)
			(*vars).C[(*vars).Q]--;
		if((*vars).B[(*vars).Q] <= -(*vars).N[(*vars).Q])
			(*vars).B[(*vars).Q] = -(*vars).N[(*vars).Q] + 1;
	}
	else if((*vars).B[(*vars).Q] > 0)
	{
		(*vars).B[(*vars).Q] -= (*vars).N[(*vars).Q];
		if((*vars).C[(*vars).Q] < (*vars).MAX_C)
			(*vars).C[(*vars).Q]++;
		if((*vars).B[(*vars).Q] > 0)
			(*vars).B[(*vars).Q] = 0;
	}
}
