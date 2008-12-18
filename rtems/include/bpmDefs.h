#ifndef BPMDEFS_H_
#define BPMDEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME -- unused */
typedef struct {
	int rawX;
	int rawY;
	double position;
	double intensity;
}Bpm;

/* conversion factors for each BPM: Volts/mm   */
double XBPM_convFactor[54]= {
	/*"1401-01:BPMx:val",*/		1.261,
	/*"1401-02:BPMx:val",*/	    1.261,
	/*"1401-03:BPMx:val",*/   	1.261,
	/*"1401-04:BPMx:val",*/   	1.261,

	/*"1402-07:BPMx:val",*/   	1.261,
	/*"1402-08:BPMx:val",*/   	1.261,
	/*"1402-09:BPMx:val",*/   	1.261,
	/*"1402-10:BPMx:val",*/   	1.261,

	/*"1403-01:BPMx:val",*/   	1.261,/*unused in orbit correction*/
	/*"1403-02:BPMx:val",*/   	1.261,
	/*"1403-03:BPMx:val",*/   	1.261,
	/*"1403-04:BPMx:val",*/   	1.261,
	/*"1403-05:BPMx:val",*/   	1.261,

	/*"1404-01:BPMx:val",*/   	1.261,/*unused in orbit correction*/
	/*"1404-02:BPMx:val",*/   	1.261,
	/*"1404-03:BPMx:val",*/   	1.261,
	/*"1404-04:BPMx:val",*/   	1.261,
	/*"1404-05:BPMx:val",*/   	1.261,

	/*"1405-01:BPMx:val",*/   	1.261,/*unused in orbit correction*/
	/*"1405-02:BPMx:val",*/   	1.261,
	/*"1405-03:BPMx:val",*/   	1.261,
	/*"1405-04:BPMx:val",*/   	1.261,
	/*"1405-05:BPMx:val",*/   	1.261,

	/*"1406-02:BPMx:val",*/   	1.261,
	/*"1406-03:BPMx:val",*/   	1.261,
	/*"1406-04:BPMx:val",*/   	1.261,
	/*"1406-05:BPMx:val",*/   	1.261,

	/*"1407-01:BPMx:val",*/   	1.261,/*unused in orbit correction*/
	/*"1407-03:BPMx:val",*/   	1.261,
	/*"1407-04:BPMx:val",*/   	1.261,
	/*"1407-05:BPMx:val",*/   	1.261,
	/*"1407-06:BPMx:val",*/   	1.261,

	/*"1408-01:BPMx:val",   	2.685,*/
	/*"1408-02:BPMx:val",*/   	1.261,
	/*"1408-03:BPMx:val",*/   	1.261,
	/*"1408-04:BPMx:val",*/   	1.261,
	/*"1408-05:BPMx:val",*/   	1.261,

	/*"1409-01:BPMx:val",*/   	1.261,/*unused in orbit correction*/
	/*"1409-02:BPMx:val",*/   	1.261,
	/*"1409-03:BPMx:val",*/   	1.261,
	/*"1409-04:BPMx:val",*/   	1.261,
	/*"1409-05:BPMx:val",*/   	1.261,

	/*"1410-01:BPMx:val",   	3.17,*/
	/*"1410-02:BPMx:val",*/   	1.261,
	/*"1410-03:BPMx:val",*/   	1.261,
	/*"1410-04:BPMx:val",*/   	1.261,
	/*"1410-05:BPMx:val",*/   	1.261,

	/*"1411-01:BPMx:val",*/   	3.17,/*unused in orbit correction*/
	/*"1411-02:BPMx:val",*/   	1.261,
	/*"1411-03:BPMx:val",*/   	1.261,
	/*"1411-04:BPMx:val",*/   	1.261,
	/*"1411-05:BPMx:val",*/   	1.261,

	/*"1412-02:BPMx:val",*/   	1.261,
	/*"1412-03:BPMx:val",*/   	1.261,
	/*"1412-04:BPMx:val",*/   	1.261,
	/*"1412-05:BPMx:val",*/   	1.261

};

double YBPM_convFactor[54]= {
	/*"1401-01:BPMy:val",*/   	1.152,
	/*"1401-02:BPMy:val",*/   	1.152,
	/*"1401-03:BPMy:val",*/   	1.152,
	/*"1401-04:BPMy:val",*/   	1.152,

	/*"1402-07:BPMy:val",*/   	1.152,
	/*"1402-08:BPMy:val",*/   	1.152,
	/*"1402-09:BPMy:val",*/   	1.152,
	/*"1402-10:BPMy:val",*/   	1.152,

	/*"1403-01:BPMy:val",*/   	1.152,/*unused in orbit correction*/
	/*"1403-02:BPMy:val",*/   	1.152,
	/*"1403-03:BPMy:val",*/   	1.152,
	/*"1403-04:BPMy:val",*/   	1.152,
	/*"1403-05:BPMy:val",*/   	1.152,

	/*"1404-01:BPMy:val",*/   	1.152,/*unused in orbit correction*/
	/*"1404-02:BPMy:val",*/   	1.152,
	/*"1404-03:BPMy:val",*/   	1.152,
	/*"1404-04:BPMy:val",*/   	1.152,
	/*"1404-05:BPMy:val",*/   	1.152,

	/*"1405-01:BPMy:val",*/   	1.152,/*unused in orbit correction*/
	/*"1405-02:BPMy:val",*/   	1.152,
	/*"1405-03:BPMy:val",*/   	1.152,
	/*"1405-04:BPMy:val",*/   	1.152,
	/*"1405-05:BPMy:val",*/   	1.152,

	/*"1406-02:BPMy:val",*/   	1.152,
	/*"1406-03:BPMy:val",*/   	1.152,
	/*"1406-04:BPMy:val",*/   	1.152,
	/*"1406-05:BPMy:val",*/   	1.152,

	/*"1407-01:BPMy:val",*/   	1.152,/*unused in orbit correction*/
	/*"1407-03:BPMy:val",*/   	1.152,
	/*"1407-04:BPMy:val",*/   	1.152,
	/*"1407-05:BPMy:val",*/   	1.152,
	/*"1407-06:BPMy:val",*/   	1.152,

	/*"1408-01:BPMy:val",   	1.722,*/
	/*"1408-02:BPMy:val",*/   	1.152,
	/*"1408-03:BPMy:val",*/   	1.152,
	/*"1408-04:BPMy:val",*/   	1.152,
	/*"1408-05:BPMy:val",*/   	1.152,

	/*"1409-01:BPMy:val",*/   	1.152,/*unused in orbit correction*/
	/*"1409-02:BPMy:val",*/   	1.152,
	/*"1409-03:BPMy:val",*/   	1.152,
	/*"1409-04:BPMy:val",*/   	1.152,
	/*"1409-05:BPMy:val",*/   	1.152,

	/*"1410-01:BPMy:val",   	2.60,*/
	/*"1410-02:BPMy:val",*/   	1.152,
	/*"1410-03:BPMy:val",*/   	1.152,
	/*"1410-04:BPMy:val",*/   	1.152,
	/*"1410-05:BPMy:val",*/   	1.152,

	/*"1411-01:BPMy:val",*/   	2.60,/*unused in orbit correction*/
	/*"1411-02:BPMy:val",*/   	1.152,
	/*"1411-03:BPMy:val",*/   	1.152,
	/*"1411-04:BPMy:val",*/   	1.152,
	/*"1411-05:BPMy:val",*/   	1.152,

	/*"1412-02:BPMy:val",*/   	1.152,
	/*"1412-03:BPMy:val",*/   	1.152,
	/*"1412-04:BPMy:val",*/   	1.152,
	/*"1412-05:BPMy:val"*/      1.152
};

#ifdef __cplusplus
}
#endif

#endif /*BPMDEFS_H_*/
