/* Machine Generated Real Split Radix Decimation in Time FFT */
#define	INVSQ2	0.70710678118654752440
void
rfft64(X)
register double X[];
{
	register double	t0, t1, c3, d3, c2, d2;
	register int	i;
/* bit reverse */
t0 = X[32];
X[32] = X[1];
X[1] = t0;
t0 = X[16];
X[16] = X[2];
X[2] = t0;
t0 = X[48];
X[48] = X[3];
X[3] = t0;
t0 = X[8];
X[8] = X[4];
X[4] = t0;
t0 = X[40];
X[40] = X[5];
X[5] = t0;
t0 = X[24];
X[24] = X[6];
X[6] = t0;
t0 = X[56];
X[56] = X[7];
X[7] = t0;
t0 = X[36];
X[36] = X[9];
X[9] = t0;
t0 = X[20];
X[20] = X[10];
X[10] = t0;
t0 = X[52];
X[52] = X[11];
X[11] = t0;
t0 = X[44];
X[44] = X[13];
X[13] = t0;
t0 = X[28];
X[28] = X[14];
X[14] = t0;
t0 = X[60];
X[60] = X[15];
X[15] = t0;
t0 = X[34];
X[34] = X[17];
X[17] = t0;
t0 = X[50];
X[50] = X[19];
X[19] = t0;
t0 = X[42];
X[42] = X[21];
X[21] = t0;
t0 = X[26];
X[26] = X[22];
X[22] = t0;
t0 = X[58];
X[58] = X[23];
X[23] = t0;
t0 = X[38];
X[38] = X[25];
X[25] = t0;
t0 = X[54];
X[54] = X[27];
X[27] = t0;
t0 = X[46];
X[46] = X[29];
X[29] = t0;
t0 = X[62];
X[62] = X[31];
X[31] = t0;
t0 = X[49];
X[49] = X[35];
X[35] = t0;
t0 = X[41];
X[41] = X[37];
X[37] = t0;
t0 = X[57];
X[57] = X[39];
X[39] = t0;
t0 = X[53];
X[53] = X[43];
X[43] = t0;
t0 = X[61];
X[61] = X[47];
X[47] = t0;
t0 = X[59];
X[59] = X[55];
X[55] = t0;
/* length two xforms */
t0 = X[0];
X[0] += X[1];
X[1] = t0 - X[1];
t0 = X[4];
X[4] += X[5];
X[5] = t0 - X[5];
t0 = X[8];
X[8] += X[9];
X[9] = t0 - X[9];
t0 = X[12];
X[12] += X[13];
X[13] = t0 - X[13];
t0 = X[16];
X[16] += X[17];
X[17] = t0 - X[17];
t0 = X[20];
X[20] += X[21];
X[21] = t0 - X[21];
t0 = X[24];
X[24] += X[25];
X[25] = t0 - X[25];
t0 = X[28];
X[28] += X[29];
X[29] = t0 - X[29];
t0 = X[32];
X[32] += X[33];
X[33] = t0 - X[33];
t0 = X[36];
X[36] += X[37];
X[37] = t0 - X[37];
t0 = X[40];
X[40] += X[41];
X[41] = t0 - X[41];
t0 = X[44];
X[44] += X[45];
X[45] = t0 - X[45];
t0 = X[48];
X[48] += X[49];
X[49] = t0 - X[49];
t0 = X[52];
X[52] += X[53];
X[53] = t0 - X[53];
t0 = X[56];
X[56] += X[57];
X[57] = t0 - X[57];
t0 = X[60];
X[60] += X[61];
X[61] = t0 - X[61];
t0 = X[6];
X[6] += X[7];
X[7] = t0 - X[7];
t0 = X[22];
X[22] += X[23];
X[23] = t0 - X[23];
t0 = X[38];
X[38] += X[39];
X[39] = t0 - X[39];
t0 = X[54];
X[54] += X[55];
X[55] = t0 - X[55];
t0 = X[30];
X[30] += X[31];
X[31] = t0 - X[31];
/* other butterflies */
t0 = X[3] + X[2];
X[3] = X[2] - X[3];
X[2] = X[0] - t0;
X[0] += t0;
t0 = X[11] + X[10];
X[11] = X[10] - X[11];
X[10] = X[8] - t0;
X[8] += t0;
t0 = X[19] + X[18];
X[19] = X[18] - X[19];
X[18] = X[16] - t0;
X[16] += t0;
t0 = X[27] + X[26];
X[27] = X[26] - X[27];
X[26] = X[24] - t0;
X[24] += t0;
t0 = X[35] + X[34];
X[35] = X[34] - X[35];
X[34] = X[32] - t0;
X[32] += t0;
t0 = X[43] + X[42];
X[43] = X[42] - X[43];
X[42] = X[40] - t0;
X[40] += t0;
t0 = X[51] + X[50];
X[51] = X[50] - X[51];
X[50] = X[48] - t0;
X[48] += t0;
t0 = X[59] + X[58];
X[59] = X[58] - X[59];
X[58] = X[56] - t0;
X[56] += t0;
t0 = X[15] + X[14];
X[15] = X[14] - X[15];
X[14] = X[12] - t0;
X[12] += t0;
t0 = X[47] + X[46];
X[47] = X[46] - X[47];
X[46] = X[44] - t0;
X[44] += t0;
t0 = X[63] + X[62];
X[63] = X[62] - X[63];
X[62] = X[60] - t0;
X[60] += t0;
t0 = X[6] + X[4];
X[6] = X[4] - X[6];
X[4] = X[0] - t0;
X[0] += t0;
t0 = X[22] + X[20];
X[22] = X[20] - X[22];
X[20] = X[16] - t0;
X[16] += t0;
t0 = X[38] + X[36];
X[38] = X[36] - X[38];
X[36] = X[32] - t0;
X[32] += t0;
t0 = X[54] + X[52];
X[54] = X[52] - X[54];
X[52] = X[48] - t0;
X[48] += t0;
t0 = X[30] + X[28];
X[30] = X[28] - X[30];
X[28] = X[24] - t0;
X[24] += t0;
t0 = (X[5]-X[7])*INVSQ2;
t1 = (X[5]+X[7])*INVSQ2;
X[5] = t1 - X[3];
X[7] = t1 + X[3];
X[3] = X[1] - t0;
X[1] += t0;
t0 = (X[21]-X[23])*INVSQ2;
t1 = (X[21]+X[23])*INVSQ2;
X[21] = t1 - X[19];
X[23] = t1 + X[19];
X[19] = X[17] - t0;
X[17] += t0;
t0 = (X[37]-X[39])*INVSQ2;
t1 = (X[37]+X[39])*INVSQ2;
X[37] = t1 - X[35];
X[39] = t1 + X[35];
X[35] = X[33] - t0;
X[33] += t0;
t0 = (X[53]-X[55])*INVSQ2;
t1 = (X[53]+X[55])*INVSQ2;
X[53] = t1 - X[51];
X[55] = t1 + X[51];
X[51] = X[49] - t0;
X[49] += t0;
t0 = (X[29]-X[31])*INVSQ2;
t1 = (X[29]+X[31])*INVSQ2;
X[29] = t1 - X[27];
X[31] = t1 + X[27];
X[27] = X[25] - t0;
X[25] += t0;
t0 = X[12] + X[8];
X[12] = X[8] - X[12];
X[8] = X[0] - t0;
X[0] += t0;
t0 = X[44] + X[40];
X[44] = X[40] - X[44];
X[40] = X[32] - t0;
X[32] += t0;
t0 = X[60] + X[56];
X[60] = X[56] - X[60];
X[56] = X[48] - t0;
X[48] += t0;
t0 = (X[10]-X[14])*INVSQ2;
t1 = (X[10]+X[14])*INVSQ2;
X[10] = t1 - X[6];
X[14] = t1 + X[6];
X[6] = X[2] - t0;
X[2] += t0;
t0 = (X[42]-X[46])*INVSQ2;
t1 = (X[42]+X[46])*INVSQ2;
X[42] = t1 - X[38];
X[46] = t1 + X[38];
X[38] = X[34] - t0;
X[34] += t0;
t0 = (X[58]-X[62])*INVSQ2;
t1 = (X[58]+X[62])*INVSQ2;
X[58] = t1 - X[54];
X[62] = t1 + X[54];
X[54] = X[50] - t0;
X[50] += t0;
c2 = X[9]*0.923879532511286738483136 - X[11]*0.382683432365089781779233;
d2 = -(X[9]*0.382683432365089781779233 + X[11]*0.923879532511286738483136);
c3 = X[13]*0.382683432365089837290384 - X[15]*0.923879532511286738483136;
d3 = -(X[13]*0.923879532511286738483136 + X[15]*0.382683432365089837290384);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[9] = -X[7] - d3;
X[11] = -X[5] + c3;
X[13] = X[5] + c3;
X[15] = X[7] - d3;
X[5] = X[3] + t1;
X[7] = X[1] - t0;
X[1] += t0;
X[3] -= t1;
c2 = X[41]*0.923879532511286738483136 - X[43]*0.382683432365089781779233;
d2 = -(X[41]*0.382683432365089781779233 + X[43]*0.923879532511286738483136);
c3 = X[45]*0.382683432365089837290384 - X[47]*0.923879532511286738483136;
d3 = -(X[45]*0.923879532511286738483136 + X[47]*0.382683432365089837290384);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[41] = -X[39] - d3;
X[43] = -X[37] + c3;
X[45] = X[37] + c3;
X[47] = X[39] - d3;
X[37] = X[35] + t1;
X[39] = X[33] - t0;
X[33] += t0;
X[35] -= t1;
c2 = X[57]*0.923879532511286738483136 - X[59]*0.382683432365089781779233;
d2 = -(X[57]*0.382683432365089781779233 + X[59]*0.923879532511286738483136);
c3 = X[61]*0.382683432365089837290384 - X[63]*0.923879532511286738483136;
d3 = -(X[61]*0.923879532511286738483136 + X[63]*0.382683432365089837290384);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[57] = -X[55] - d3;
X[59] = -X[53] + c3;
X[61] = X[53] + c3;
X[63] = X[55] - d3;
X[53] = X[51] + t1;
X[55] = X[49] - t0;
X[49] += t0;
X[51] -= t1;
t0 = X[24] + X[16];
X[24] = X[16] - X[24];
X[16] = X[0] - t0;
X[0] += t0;
t0 = (X[20]-X[28])*INVSQ2;
t1 = (X[20]+X[28])*INVSQ2;
X[20] = t1 - X[12];
X[28] = t1 + X[12];
X[12] = X[4] - t0;
X[4] += t0;
c2 = X[17]*0.980785280403230430579242 - X[23]*0.195090322016128275839364;
d2 = -(X[17]*0.195090322016128275839364 + X[23]*0.980785280403230430579242);
c3 = X[25]*0.831469612302545235671403 - X[31]*0.555570233019602177648721;
d3 = -(X[25]*0.555570233019602177648721 + X[31]*0.831469612302545235671403);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[17] = -X[15] - d3;
X[23] = -X[9] + c3;
X[25] = X[9] + c3;
X[31] = X[15] - d3;
X[9] = X[7] + t1;
X[15] = X[1] - t0;
X[1] += t0;
X[7] -= t1;
c2 = X[18]*0.923879532511286738483136 - X[22]*0.382683432365089781779233;
d2 = -(X[18]*0.382683432365089781779233 + X[22]*0.923879532511286738483136);
c3 = X[26]*0.382683432365089837290384 - X[30]*0.923879532511286738483136;
d3 = -(X[26]*0.923879532511286738483136 + X[30]*0.382683432365089837290384);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[18] = -X[14] - d3;
X[22] = -X[10] + c3;
X[26] = X[10] + c3;
X[30] = X[14] - d3;
X[10] = X[6] + t1;
X[14] = X[2] - t0;
X[2] += t0;
X[6] -= t1;
c2 = X[19]*0.831469612302545235671403 - X[21]*0.555570233019602177648721;
d2 = -(X[19]*0.555570233019602177648721 + X[21]*0.831469612302545235671403);
c3 = X[27]*-0.195090322016128220328213 - X[29]*0.980785280403230430579242;
d3 = -(X[27]*0.980785280403230430579242 + X[29]*-0.195090322016128220328213);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[19] = -X[13] - d3;
X[21] = -X[11] + c3;
X[27] = X[11] + c3;
X[29] = X[13] - d3;
X[11] = X[5] + t1;
X[13] = X[3] - t0;
X[3] += t0;
X[5] -= t1;
t0 = X[48] + X[32];
X[48] = X[32] - X[48];
X[32] = X[0] - t0;
X[0] += t0;
t0 = (X[40]-X[56])*INVSQ2;
t1 = (X[40]+X[56])*INVSQ2;
X[40] = t1 - X[24];
X[56] = t1 + X[24];
X[24] = X[8] - t0;
X[8] += t0;
c2 = X[33]*0.995184726672196928731751 - X[47]*0.0980171403295606036287779;
d2 = -(X[33]*0.0980171403295606036287779 + X[47]*0.995184726672196928731751);
c3 = X[49]*0.956940335732208824381928 - X[63]*0.290284677254462386564171;
d3 = -(X[49]*0.290284677254462386564171 + X[63]*0.956940335732208824381928);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[33] = -X[31] - d3;
X[47] = -X[17] + c3;
X[49] = X[17] + c3;
X[63] = X[31] - d3;
X[17] = X[15] + t1;
X[31] = X[1] - t0;
X[1] += t0;
X[15] -= t1;
c2 = X[34]*0.980785280403230430579242 - X[46]*0.195090322016128275839364;
d2 = -(X[34]*0.195090322016128275839364 + X[46]*0.980785280403230430579242);
c3 = X[50]*0.831469612302545235671403 - X[62]*0.555570233019602177648721;
d3 = -(X[50]*0.555570233019602177648721 + X[62]*0.831469612302545235671403);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[34] = -X[30] - d3;
X[46] = -X[18] + c3;
X[50] = X[18] + c3;
X[62] = X[30] - d3;
X[18] = X[14] + t1;
X[30] = X[2] - t0;
X[2] += t0;
X[14] -= t1;
c2 = X[35]*0.956940335732208824381928 - X[45]*0.290284677254462386564171;
d2 = -(X[35]*0.290284677254462386564171 + X[45]*0.956940335732208824381928);
c3 = X[51]*0.63439328416364548779427 - X[61]*0.773010453362736993376814;
d3 = -(X[51]*0.773010453362736993376814 + X[61]*0.63439328416364548779427);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[35] = -X[29] - d3;
X[45] = -X[19] + c3;
X[51] = X[19] + c3;
X[61] = X[29] - d3;
X[19] = X[13] + t1;
X[29] = X[3] - t0;
X[3] += t0;
X[13] -= t1;
c2 = X[36]*0.923879532511286738483136 - X[44]*0.382683432365089781779233;
d2 = -(X[36]*0.382683432365089781779233 + X[44]*0.923879532511286738483136);
c3 = X[52]*0.382683432365089837290384 - X[60]*0.923879532511286738483136;
d3 = -(X[52]*0.923879532511286738483136 + X[60]*0.382683432365089837290384);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[36] = -X[28] - d3;
X[44] = -X[20] + c3;
X[52] = X[20] + c3;
X[60] = X[28] - d3;
X[20] = X[12] + t1;
X[28] = X[4] - t0;
X[4] += t0;
X[12] -= t1;
c2 = X[37]*0.881921264348355049556005 - X[43]*0.471396736825997642039709;
d2 = -(X[37]*0.471396736825997642039709 + X[43]*0.881921264348355049556005);
c3 = X[53]*0.0980171403295607701622316 - X[59]*0.995184726672196817709448;
d3 = -(X[53]*0.995184726672196817709448 + X[59]*0.0980171403295607701622316);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[37] = -X[27] - d3;
X[43] = -X[21] + c3;
X[53] = X[21] + c3;
X[59] = X[27] - d3;
X[21] = X[11] + t1;
X[27] = X[5] - t0;
X[5] += t0;
X[11] -= t1;
c2 = X[38]*0.831469612302545235671403 - X[42]*0.555570233019602177648721;
d2 = -(X[38]*0.555570233019602177648721 + X[42]*0.831469612302545235671403);
c3 = X[54]*-0.195090322016128220328213 - X[58]*0.980785280403230430579242;
d3 = -(X[54]*0.980785280403230430579242 + X[58]*-0.195090322016128220328213);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[38] = -X[26] - d3;
X[42] = -X[22] + c3;
X[54] = X[22] + c3;
X[58] = X[26] - d3;
X[22] = X[10] + t1;
X[26] = X[6] - t0;
X[6] += t0;
X[10] -= t1;
c2 = X[39]*0.773010453362736993376814 - X[41]*0.63439328416364548779427;
d2 = -(X[39]*0.63439328416364548779427 + X[41]*0.773010453362736993376814);
c3 = X[55]*-0.471396736825997697550861 - X[57]*0.881921264348355049556005;
d3 = -(X[55]*0.881921264348355049556005 + X[57]*-0.471396736825997697550861);
t0 = c2 + c3;
c3 = c2 - c3;
t1 = d2 - d3;
d3 += d2;
X[39] = -X[25] - d3;
X[41] = -X[23] + c3;
X[55] = X[23] + c3;
X[57] = X[25] - d3;
X[23] = X[9] + t1;
X[25] = X[7] - t0;
X[7] += t0;
X[9] -= t1;
/* reverse Imag part! */
for( i = 64/2+1; i < 64; i++ )
	X[i] = -X[i];
}
