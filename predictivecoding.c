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

#include "predictivecoding.h"
static inline uint32 min(uint32,uint32);
static inline uint32 max(uint32,uint32);

 static inline uint32 min(uint32 a,uint32 b){ return (a > b) ? b : a; }
  static inline uint32 max(uint32 a,uint32 b){ return (a > b) ? a : b; }


void prev_context(codingvars* vars, parameters params, image_data* pre_im_data)
{
	(*vars).Rbm0 = ((*vars).row - (*vars).m0==0) ? 0 : pre_im_data->image[(*vars).comp][(*vars).row- (*vars).m0-1][(*vars).col -(*vars).n0];
	(*vars).Rdm0 = (((*vars).col - (*vars).n0)==pre_im_data->width-1 || (*vars).row - (*vars).m0==0) ? (*vars).Rbm0 : pre_im_data->image[(*vars).comp][(*vars).row - (*vars).m0-1][(*vars).col - (*vars).n0+1];
	if((*vars).col - (*vars).n0>0)
		(*vars).Rcm0 = (((*vars).row - (*vars).m0)==0) ? 0 : pre_im_data->image[(*vars).comp][(*vars).row - (*vars).m0-1][(*vars).col - (*vars).n0-1];
	else
		(*vars).Rcm0 = (*vars).prevRam0;
	(*vars).Ram0 = (((*vars).col- (*vars).n0)==0) ? (*vars).prevRam0=(*vars).Rbm0 : pre_im_data->image[(*vars).comp][(*vars).row - (*vars).m0][(*vars).col-(*vars).n0-1];

	(*vars).Ixm0 = pre_im_data->image[(*vars).comp][(*vars).row - (*vars).m0][(*vars).col - (*vars).n0];
}



void context_determination(codingvars* vars, parameters params, image_data* im_data)
{

	int32 D1, D2, D3;			// local gradients
	int8 Q1, Q2, Q3;			// region numbers to quantize local gradients

	// causal template construction
	// c b d
	// a x


	(*vars).Rb = ((*vars).row==0) ? 0 : im_data->image[(*vars).comp][(*vars).row-1][(*vars).col];
	(*vars).Rd = ((*vars).col==im_data->width-1 || (*vars).row==0) ? (*vars).Rb : im_data->image[(*vars).comp][(*vars).row-1][(*vars).col+1];
	if((*vars).col>0)
		(*vars).Rc = ((*vars).row==0) ? 0 : im_data->image[(*vars).comp][(*vars).row-1][(*vars).col-1];
	else
		(*vars).Rc = (*vars).prevRa;
	(*vars).Ra = ((*vars).col==0) ? (*vars).prevRa=(*vars).Rb : im_data->image[(*vars).comp][(*vars).row][(*vars).col-1];

	(*vars).Ix = im_data->image[(*vars).comp][(*vars).row][(*vars).col];

	/* A.3.1 Local gradient computation */

	D1 = (*vars).Rd - (*vars).Rb;
	D2 = (*vars).Rb - (*vars).Rc;
	D3 = (*vars).Rc - (*vars).Ra;

	/* A.3.2 Mode selection */

	if((abs(D1) <= params.NEAR)&&(abs(D2) <= params.NEAR)&&(abs(D3) <= params.NEAR))
		//(*vars).RunModeProcessing = true;
		(*vars).RunModeProcessing = false;
	else
		(*vars).RunModeProcessing = false;

(*vars).RunModeProcessing = false;

	if(!(*vars).RunModeProcessing)	// regular mode
	{
		/* A.3.3 Local gradients quantization */

		if      (D1 <= -params.T3)   	Q1 =-4;
		else if (D1 <= -params.T2)   	Q1 =-3;
		else if (D1 <= -params.T1)   	Q1 =-2;
		else if (D1 <  -params.NEAR) 	Q1 =-1;
		else if (D1 <=  params.NEAR) 	Q1 = 0;
		else if (D1 <   params.T1)   	Q1 = 1;
		else if (D1 <   params.T2)   	Q1 = 2;
		else if (D1 <   params.T3)   	Q1 = 3;
		else                  		Q1 = 4;

		if      (D2 <= -params.T3)   	Q2 =-4;
		else if (D2 <= -params.T2)   	Q2 =-3;
		else if (D2 <= -params.T1)   	Q2 =-2;
		else if (D2 <  -params.NEAR) 	Q2 =-1;
		else if (D2 <=  params.NEAR) 	Q2 = 0;
		else if (D2 <   params.T1)   	Q2 = 1;
		else if (D2 <   params.T2)   	Q2 = 2;
		else if (D2 <   params.T3)   	Q2 = 3;
		else                  		Q2 = 4;

		if      (D3 <= -params.T3)   	Q3=-4;
		else if (D3 <= -params.T2)   	Q3=-3;
		else if (D3 <= -params.T1)   	Q3=-2;
		else if (D3 <  -params.NEAR) 	Q3=-1;
		else if (D3 <=  params.NEAR) 	Q3= 0;
		else if (D3 <   params.T1)   	Q3= 1;
		else if (D3 <   params.T2)   	Q3= 2;
		else if (D3 <   params.T3)   	Q3= 3;
		else 				Q3= 4;

		/* A.3.4 Quantized gradient merging */

		if( (Q1<0) || (Q1==0 && Q2<0) || (Q1==0 && Q2==0 && Q3<0) )
		{
			(*vars).SIGN = -1;
			Q1 = -Q1;
			Q2 = -Q2;
			Q3 = -Q3;
		}
		else
			(*vars).SIGN = 1;

		// one-to-one mapping of the vector (Q1,Q2,Q3) to the integer Q
		if (Q1 == 0)
		{
			if (Q2 == 0)
				(*vars).Q=360+Q3;
			else
				(*vars).Q=324+(Q2-1)*9+(Q3+4);
		}
		else
			(*vars).Q=(Q1-1)*81+(Q2+4)*9+(Q3+4);
	}
}

void predict_sample_value(codingvars* vars, parameters params)
{
	/* A.4.1 Edge-detecting predictor */
	bool predictor_c;
	predictor_c = true;//true or false

uint8 t1 = 10;
uint8 t2 = 20;
//uint8 wa = 10;
//uint8 wb = 8;
//uint8 wc = 2;
//uint8 wd = 1;

if(predictor_c){
	if((*vars).Rc>=max((*vars).Ra,(*vars).Rb))
		(*vars).Px = min((*vars).Ra,(*vars).Rb);
	else
	{
		if((*vars).Rc<=min((*vars).Ra,(*vars).Rb))
			(*vars).Px = max((*vars).Ra,(*vars).Rb);
		else
			(*vars).Px = (*vars).Ra + (*vars).Rb - (*vars).Rc;
	}
}
else{
	if((*vars).Rc>=max((*vars).Ra,(*vars).Rb))
	  {
	    if((*vars).Ra <= (*vars).Rb)
	      (*vars).Px = (*vars).Ra;
	    else if(((*vars).Rc - max((*vars).Ra,(*vars).Rb)) > t1 && abs((*vars).Ra - (*vars).Rb) <= t2)
	      //(*vars).Px = (a1 * (*vars).Rd + a2 * (*vars).Rb) / (a1 + a2);
	      (*vars).Px = ((*vars).Ra + (*vars).Rb + (*vars).Rd) / 3;
	      //(*vars).Px = (wa * (*vars).Ra + wb * (*vars).Rb + wc * (*vars).Rc + wd * (*vars).Rd) / (wa + wb + wc + wd);
	    else
	      (*vars).Px = (*vars).Rb;
	  }
	else if((*vars).Rc<=min((*vars).Ra,(*vars).Rb))
	  {
	    if((*vars).Ra >= (*vars).Rb)
	      (*vars).Px = (*vars).Ra;
	    //else if(((*vars).Rd - (*vars).Rb) >= t1 && ((*vars).Ra - (*vars).Rc) >= t2)
	    else if((min((*vars).Ra,(*vars).Rb) - (*vars).Rc) > t1 && abs((*vars).Ra - (*vars).Rb) <= t2)
	      //(*vars).Px = (a1 * (*vars).Rd + a2 * (*vars).Rb) / (a1 + a2);
	      (*vars).Px = ((*vars).Ra + (*vars).Rb + (*vars).Rd) / 3;
	      //(*vars).Px = (wa * (*vars).Ra + wb * (*vars).Rb + wc * (*vars).Rc + wd * (*vars).Rd) / (wa + wb + wc + wd);
	    else
	      (*vars).Px = (*vars).Rb;
	  }
	  else
	    (*vars).Px = (*vars).Ra + (*vars).Rb - (*vars).Rc;

}
	/* A.4.2 Prediction correction */
	(*vars).PxNO = (*vars).Px;

	(*vars).Px += ((*vars).SIGN == 1)? (*vars).C[(*vars).Q] : -(*vars).C[(*vars).Q];

	if((*vars).Px>params.MAXVAL)
		(*vars).Px = params.MAXVAL;
	else if((*vars).Px<0)
		(*vars).Px = 0;
}

void predictor1(codingvars* vars, parameters params)
{
	int32 xk1,xk2,xk3;
	xk1 = (*vars).Ixm0 + (*vars).Rb - (*vars).Rbm0;
	xk2 = (*vars).Ixm0 + (*vars).Ra - (*vars).Ram0;
	xk3 = (*vars).Ra + (*vars).Rb - (*vars).Rcm0;

	(*vars).Px1 = floor((xk1 + xk2 + xk3) / 3);

	(*vars).Px1 += ((*vars).SIGN == 1)? (*vars).C[(*vars).Q] : -(*vars).C[(*vars).Q];

	if((*vars).Px1>params.MAXVAL)
		(*vars).Px1 = params.MAXVAL;
	else if((*vars).Px1<0)
		(*vars).Px1 = 0;

}

void predictor2(codingvars* vars, parameters params)
{
	(*vars).Px2 = (*vars).Ixm0 - (*vars).Ram0 + (*vars).Rbm0 - (*vars).Rcm0 + (*vars).Ra + (*vars).Rb - (*vars).Rc;

	(*vars).Px2 += ((*vars).SIGN == 1)? (*vars).C[(*vars).Q] : -(*vars).C[(*vars).Q];

	if((*vars).Px2>params.MAXVAL)
		(*vars).Px2 = params.MAXVAL;
	else if((*vars).Px2<0)
		(*vars).Px2 = 0;
}

void motion_vector(codingvars* vars, parameters params, image_data* pre_im_data, image_data* im_data)
{
	/*	11	8		6		9		12
			7		3		2		4
			5		1		Ix
	*/
	uint32 Dif, Dif_min;
	Dif_min = 2048;
	uint16 I1,I2,I3,I5,I6,I7,I8,I11;
	int8 h, v;

	for(h = -(*vars).mv; h <= (*vars).mv; h++)
		for(v = -(*vars).nv; v <= (*vars).nv; v++)
		{
			//if((((*vars).row -2 + h) >= 0) && (((*vars).col-2 + h) >= 0) && (((*vars).row + h) <= im_data->height) && (((*vars).col + v) <= im_data->width) && (((*vars).row-2) >= 0) && (((*vars).col-2) >= 0))
			//if(((*vars).row >= ((*vars).mv + 2)) && ((*vars).row <= (im_data->height - (*vars).mv)) && ((*vars).col >= ((*vars).nv + 2)) && ((*vars).col <= (im_data->width - (*vars).nv)))
			if(((*vars).row >= 4) && ((*vars).row <= (im_data->height - 3)) && ((*vars).col >= 5) && ((*vars).col <= (im_data->width - 3)))

			{
				I1 = abs(im_data->image[(*vars).comp][(*vars).row][(*vars).col-1] - pre_im_data->image[(*vars).comp][(*vars).row + h][(*vars).col-1 + v]);
				I2 = abs(im_data->image[(*vars).comp][(*vars).row-1][(*vars).col] - pre_im_data->image[(*vars).comp][(*vars).row-1 + h][(*vars).col + v]);
				I3 = abs(im_data->image[(*vars).comp][(*vars).row-1][(*vars).col-1] - pre_im_data->image[(*vars).comp][(*vars).row-1 + h][(*vars).col-1 + v]);
				I5 = abs(im_data->image[(*vars).comp][(*vars).row][(*vars).col-2] - pre_im_data->image[(*vars).comp][(*vars).row + h][(*vars).col-2 + v]);
				I6 = abs(im_data->image[(*vars).comp][(*vars).row-2][(*vars).col] - pre_im_data->image[(*vars).comp][(*vars).row-2 + h][(*vars).col + v]);
				I7 = abs(im_data->image[(*vars).comp][(*vars).row-1][(*vars).col-2] - pre_im_data->image[(*vars).comp][(*vars).row-1 + h][(*vars).col-2 + v]);
				I8 = abs(im_data->image[(*vars).comp][(*vars).row-2][(*vars).col-1] - pre_im_data->image[(*vars).comp][(*vars).row-2 + h][(*vars).col-1 + v]);
				I11 = abs(im_data->image[(*vars).comp][(*vars).row-2][(*vars).col-2] - pre_im_data->image[(*vars).comp][(*vars).row-2 + h][(*vars).col-2 + v]);
				Dif = I1 + I2 + I3 + I5 + I6 + I7 + I8 + I11;
				if(((*vars).row == 100) && ((*vars).col == 200))
					printf("Dif is %ld.\n", Dif);
				if(Dif_min >= Dif)
				{

					Dif_min = Dif;
					(*vars).temp = Dif_min;
					(*vars).m0 = h;
					(*vars).n0 = v;
				}
			}
			else
			{
				(*vars).m0 = 0;
				(*vars).n0 = 0;
			}
		}
}


void encode_prediction_error_intra(codingvars* vars, parameters params)
{
	(*vars).Errval_temp1 = (*vars).Ix - (*vars).Px1;
	(*vars).Errval_temp2 = (*vars).Ix - (*vars).Px2;
}


void encode_prediction_error(codingvars* vars, parameters params, image_data* im_data)
{
	/* A.4.2 Computation of prediction error */
  //(*vars).Errval_temp3 = (*vars).Ix - (*vars).PxNO;//NO C[Q] buchang Error
	(*vars).Errval = (*vars).Ix - (*vars).Px;
  (*vars).Errval_temp3 = (*vars).Errval;
	if((*vars).SIGN==-1)
		(*vars).Errval = -(*vars).Errval;

	/* A.4.4 Error quantization for near-lossless coding, and reconstructed value computation */

	if((*vars).Errval>0)
		(*vars).Errval = ((*vars).Errval + params.NEAR)/(2*params.NEAR + 1);
	else
		(*vars).Errval = -(params.NEAR - (*vars).Errval)/(2*params.NEAR + 1);

	(*vars).Rx = (*vars).Px + (*vars).SIGN*(*vars).Errval*(2*params.NEAR + 1);
	if((*vars).Rx<0)
		(*vars).Rx = 0;
	else if((*vars).Rx>params.MAXVAL)
		(*vars).Rx = params.MAXVAL;
	im_data->image[(*vars).comp][(*vars).row][(*vars).col] = (*vars).Rx;

	// modulo reduction of the error
	if((*vars).Errval<0)
		(*vars).Errval = (*vars).Errval + (*vars).RANGE;
	if((*vars).Errval>=(((*vars).RANGE + 1)/2))
		(*vars).Errval = (*vars).Errval - (*vars).RANGE;

	/* A.5 Prediction error encoding */

	/* A.5.1 Golomb coding variable computation */

	for((*vars).k=0;((*vars).N[(*vars).Q]<<(*vars).k)<(*vars).A[(*vars).Q];(*vars).k++);

	/* A.5.2 Error mapping */

	if((params.NEAR==0)&&((*vars).k==0)&&(2*(*vars).B[(*vars).Q]<=-(*vars).N[(*vars).Q]))
		if((*vars).Errval>=0)
			(*vars).MErrval = 2*(*vars).Errval + 1;
		else
			(*vars).MErrval = -2*((*vars).Errval + 1);
	else
		if((*vars).Errval>=0)
			(*vars).MErrval = 2*(*vars).Errval;
		else
			(*vars).MErrval = -2*(*vars).Errval -1;

	/* A.5.3 Mapped-error encoding */

	limited_length_Golomb_encode((*vars).MErrval, (*vars).k, (*vars).LIMIT, (*vars).qbpp);

}


void decode_prediction_error(codingvars* vars, parameters params, image_data* im_data)
{
	/* A.5.1 Golomb coding variable computation */

	int32 ErrvalAfterMR;

	for((*vars).k=0;((*vars).N[(*vars).Q]<<(*vars).k)<(*vars).A[(*vars).Q];(*vars).k++);

	/* Mapped-error decoding */

	(*vars).MErrval = limited_length_Golomb_decode((*vars).k, (*vars).LIMIT, (*vars).qbpp);

	/* Inverse Error mapping */

	if((params.NEAR==0)&&((*vars).k==0)&&(2*(*vars).B[(*vars).Q]<=-(*vars).N[(*vars).Q]))
		if((*vars).MErrval%2==0)
			(*vars).Errval = -((int32)(*vars).MErrval / 2) - 1;
		else
			(*vars).Errval = ((int32)(*vars).MErrval - 1) / 2;
	else
		if((*vars).MErrval%2==0)
			(*vars).Errval = (int32)(*vars).MErrval / 2;
		else
			(*vars).Errval = -((int32)(*vars).MErrval + 1) / 2;

	ErrvalAfterMR = (*vars).Errval;

	(*vars).Errval = (*vars).Errval * (int32)(2*params.NEAR + 1);

	if((*vars).SIGN==-1)
		(*vars).Errval = -(*vars).Errval;

	(*vars).Rx = ((*vars).Errval + (*vars).Px) % ( (*vars).RANGE*(2*params.NEAR+1) );

	if( (*vars).Rx < -params.NEAR)
		(*vars).Rx = (*vars).Rx + (*vars).RANGE*(2*params.NEAR+1);
	else if( (*vars).Rx > params.MAXVAL + params.NEAR)
		(*vars).Rx = (*vars).Rx - (*vars).RANGE*(2*params.NEAR+1);

	if( (*vars).Rx<0 )
		(*vars).Rx = 0;
	else if( (*vars).Rx>params.MAXVAL)
		(*vars).Rx = params.MAXVAL;

	im_data->image[(*vars).comp][(*vars).row][(*vars).col] = (*vars).Rx;
	(*vars).Errval = ErrvalAfterMR;

}


void encode_run(codingvars* vars, parameters params, image_data* im_data)
{
	/* A.7.1 Run scanning and run-length coding */

	(*vars).RUNval = (*vars).Ra;
	(*vars).RUNcnt = 0;
	while(abs((*vars).Ix - (*vars).RUNval) <= params.NEAR)
	{
		(*vars).RUNcnt += 1;
		(*vars).Rx = (*vars).RUNval;
		if((*vars).col == (im_data->width-1))
			break;
		else
		{
			(*vars).col++;
			(*vars).Ix = im_data->image[(*vars).comp][(*vars).row][(*vars).col];
		}
	}

	/* A.7.1.2 Run-length coding */

	while((*vars).RUNcnt >= (1<<(*vars).J[(*vars).RUNindex]))
	{
		append_bit(1);
		(*vars).RUNcnt -= (1<<(*vars).J[(*vars).RUNindex]);
		if((*vars).RUNindex<31)
			(*vars).RUNindex += 1;
	}

	(*vars).RUNindex_val = (*vars).RUNindex;

	if(abs((*vars).Ix - (*vars).RUNval) > params.NEAR)
	{
		append_bit(0);
		append_bits((*vars).RUNcnt,(*vars).J[(*vars).RUNindex]);
		if((*vars).RUNindex > 0)
			(*vars).RUNindex -= 1;
	}
	else if((*vars).RUNcnt>0)
		append_bit(1);

	/* A.7.2 Run interruption sample encoding */

	// index computation
	if(abs((*vars).Ra - (*vars).Rb) <= params.NEAR)
		(*vars).RItype = 1;
	else
		(*vars).RItype = 0;

	// prediction error for a run interruption sample
	if((*vars).RItype == 1)
		(*vars).Px = (*vars).Ra;
	else
		(*vars).Px = (*vars).Rb;
	(*vars).Errval = (*vars).Ix - (*vars).Px;

	// error computation for a run interruption sample
	if(((*vars).RItype == 0) && ((*vars).Ra > (*vars).Rb))
	{
		(*vars).Errval = -(*vars).Errval;
		(*vars).SIGN = -1;
	}
	else
		(*vars).SIGN = 1;

	if(params.NEAR > 0)
	{
		// error quantization
		if((*vars).Errval>0)
			(*vars).Errval = ((*vars).Errval + params.NEAR)/(2*params.NEAR + 1);
		else
			(*vars).Errval = -(params.NEAR - (*vars).Errval)/(2*params.NEAR + 1);

		// reconstructed value computation
		(*vars).Rx = (*vars).Px + (*vars).SIGN*(*vars).Errval*(2*params.NEAR + 1);
		if((*vars).Rx<0)
			(*vars).Rx = 0;
		else if((*vars).Rx>params.MAXVAL)
			(*vars).Rx = params.MAXVAL;
		im_data->image[(*vars).comp][(*vars).row][(*vars).col] = (*vars).Rx;
	}

	// modulo reduction of the error
	if((*vars).Errval<0)
		(*vars).Errval = (*vars).Errval + (*vars).RANGE;
	if((*vars).Errval>=(((*vars).RANGE + 1)/2))
		(*vars).Errval = (*vars).Errval - (*vars).RANGE;

	// computation of the auxiliary variable TEMP
	if((*vars).RItype == 0)
		(*vars).TEMP = (*vars).A[365];
	else
		(*vars).TEMP = (*vars).A[366] + ((*vars).N[366]>>1);

	// Golomb coding variable computation
	(*vars).Q = (*vars).RItype + 365;
	for((*vars).k=0;((*vars).N[(*vars).Q]<<(*vars).k)<(*vars).TEMP;(*vars).k++);

	// computation of map for Errval mapping
	if(((*vars).k==0)&&((*vars).Errval>0)&&(2*(*vars).Nn[(*vars).Q-365]<(*vars).N[(*vars).Q]))
		(*vars).map = 1;
	else if(((*vars).Errval<0)&&(2*(*vars).Nn[(*vars).Q-365]>=(*vars).N[(*vars).Q]))
		(*vars).map = 1;
	else if(((*vars).Errval<0)&&((*vars).k!=0))
		(*vars).map = 1;
	else
		(*vars).map = 0;

	// Errval mapping for run interruption sample
	(*vars).EMErrval = 2*abs((*vars).Errval) - (*vars).RItype - (*vars).map;

	// limited length Golomb encoding
	limited_length_Golomb_encode((*vars).EMErrval, (*vars).k, (*vars).LIMIT - (*vars).J[(*vars).RUNindex_val] - 1, (*vars).qbpp);

	// update of variables for run interruption sample
	if((*vars).Errval<0)
		(*vars).Nn[(*vars).Q-365] = (*vars).Nn[(*vars).Q-365] + 1;
	(*vars).A[(*vars).Q] = (*vars).A[(*vars).Q] + (((*vars).EMErrval + 1 + (*vars).RItype)>>1);
	if((*vars).N[(*vars).Q] == params.RESET)
	{
		(*vars).A[(*vars).Q] = (*vars).A[(*vars).Q]>>1;
		(*vars).N[(*vars).Q] = (*vars).N[(*vars).Q]>>1;
		(*vars).Nn[(*vars).Q-365] = (*vars).Nn[(*vars).Q-365]>>1;
	}
}


void decode_run(codingvars* vars, parameters params, image_data* im_data)
{
	if( read_bit()==1)
	{
		;
	}
	else
	{
		;
	}
}
