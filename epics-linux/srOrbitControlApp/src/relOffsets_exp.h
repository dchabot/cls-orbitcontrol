/* 1 designates that this BPM is IN the correction algorythm, 0 its not */
/* used to determine which bpm's to use when calculating the RMS of the orbit */
 /* BPM positions relative to the offsets */
double RelBPMData[112];
assign RelBPMData to {
	"x_1401-01:BPMx:frmOff:val",
	"x_1401-02:BPMx:frmOff:val",
	"x_1401-03:BPMx:frmOff:val",
	"x_1401-04:BPMx:frmOff:val",
	                  
	"x_1402-07:BPMx:frmOff:val",
	"x_1402-08:BPMx:frmOff:val",
	"x_1402-09:BPMx:frmOff:val",
	"x_1402-10:BPMx:frmOff:val",
	                  
	"x_1403-01:BPMx:frmOff:val",
	"x_1403-02:BPMx:frmOff:val",
	"x_1403-03:BPMx:frmOff:val",
	"x_1403-04:BPMx:frmOff:val",
	"x_1403-05:BPMx:frmOff:val",
	           
	"x_1404-01:BPMx:frmOff:val",
	"x_1404-02:BPMx:frmOff:val",
	"x_1404-03:BPMx:frmOff:val",
	"x_1404-04:BPMx:frmOff:val",
	"x_1404-05:BPMx:frmOff:val",
	           
	"x_1405-01:BPMx:frmOff:val",
	"x_1405-02:BPMx:frmOff:val",
	"x_1405-03:BPMx:frmOff:val",
	"x_1405-04:BPMx:frmOff:val",
	"x_1405-05:BPMx:frmOff:val",
	           
	"x_1406-02:BPMx:frmOff:val",
	"x_1406-03:BPMx:frmOff:val",
	"x_1406-04:BPMx:frmOff:val",
	"x_1406-05:BPMx:frmOff:val",
	           
	"x_1407-01:BPMx:frmOff:val",
	"x_1407-03:BPMx:frmOff:val",
	"x_1407-04:BPMx:frmOff:val",
	"x_1407-05:BPMx:frmOff:val",
	"x_1407-06:BPMx:frmOff:val",
	           
	"x_1408-01:BPMx:frmOff:val",
	"x_1408-02:BPMx:frmOff:val",
	"x_1408-03:BPMx:frmOff:val",
	"x_1408-04:BPMx:frmOff:val",
	"x_1408-05:BPMx:frmOff:val",
	           
	"x_1409-01:BPMx:frmOff:val",
	"x_1409-02:BPMx:frmOff:val",
	"x_1409-03:BPMx:frmOff:val",
	"x_1409-04:BPMx:frmOff:val",
	"x_1409-05:BPMx:frmOff:val",
	           
	"x_1410-01:BPMx:frmOff:val",
	"x_1410-02:BPMx:frmOff:val",
	"x_1410-03:BPMx:frmOff:val",
	"x_1410-04:BPMx:frmOff:val",
	"x_1410-05:BPMx:frmOff:val",
	           
	"x_1411-01:BPMx:frmOff:val",
	"x_1411-02:BPMx:frmOff:val",
	"x_1411-03:BPMx:frmOff:val",
	"x_1411-04:BPMx:frmOff:val",
	"x_1411-05:BPMx:frmOff:val",
	           
	"x_1412-02:BPMx:frmOff:val",
	"x_1412-03:BPMx:frmOff:val",
	"x_1412-04:BPMx:frmOff:val",
	"x_1412-05:BPMx:frmOff:val",
	
	
	"x_1401-01:BPMy:frmOff:val",
	"x_1401-02:BPMy:frmOff:val",
	"x_1401-03:BPMy:frmOff:val",
	"x_1401-04:BPMy:frmOff:val",
	                  
	"x_1402-07:BPMy:frmOff:val",
	"x_1402-08:BPMy:frmOff:val",
	"x_1402-09:BPMy:frmOff:val",
	"x_1402-10:BPMy:frmOff:val",
	                  
	"x_1403-01:BPMy:frmOff:val",
	"x_1403-02:BPMy:frmOff:val",
	"x_1403-03:BPMy:frmOff:val",
	"x_1403-04:BPMy:frmOff:val",
	"x_1403-05:BPMy:frmOff:val",
	           
	"x_1404-01:BPMy:frmOff:val",
	"x_1404-02:BPMy:frmOff:val",
	"x_1404-03:BPMy:frmOff:val",
	"x_1404-04:BPMy:frmOff:val",
	"x_1404-05:BPMy:frmOff:val",
	           
	"x_1405-01:BPMy:frmOff:val",
	"x_1405-02:BPMy:frmOff:val",
	"x_1405-03:BPMy:frmOff:val",
	"x_1405-04:BPMy:frmOff:val",
	"x_1405-05:BPMy:frmOff:val",
	           
	"x_1406-02:BPMy:frmOff:val",
	"x_1406-03:BPMy:frmOff:val",
	"x_1406-04:BPMy:frmOff:val",
	"x_1406-05:BPMy:frmOff:val",
	           
	"x_1407-01:BPMy:frmOff:val",
	"x_1407-03:BPMy:frmOff:val",
	"x_1407-04:BPMy:frmOff:val",
	"x_1407-05:BPMy:frmOff:val",
	"x_1407-06:BPMy:frmOff:val",
	           
	"x_1408-01:BPMy:frmOff:val",
	"x_1408-02:BPMy:frmOff:val",
	"x_1408-03:BPMy:frmOff:val",
	"x_1408-04:BPMy:frmOff:val",
	"x_1408-05:BPMy:frmOff:val",
	           
	"x_1409-01:BPMy:frmOff:val",
	"x_1409-02:BPMy:frmOff:val",
	"x_1409-03:BPMy:frmOff:val",
	"x_1409-04:BPMy:frmOff:val",
	"x_1409-05:BPMy:frmOff:val",
	           
	"x_1410-01:BPMy:frmOff:val",
	"x_1410-02:BPMy:frmOff:val",
	"x_1410-03:BPMy:frmOff:val",
	"x_1410-04:BPMy:frmOff:val",
	"x_1410-05:BPMy:frmOff:val",
	           
	"x_1411-01:BPMy:frmOff:val",
	"x_1411-02:BPMy:frmOff:val",
	"x_1411-03:BPMy:frmOff:val",
	"x_1411-04:BPMy:frmOff:val",
	"x_1411-05:BPMy:frmOff:val",
	           
	"x_1412-02:BPMy:frmOff:val",
	"x_1412-03:BPMy:frmOff:val",
	"x_1412-04:BPMy:frmOff:val",
	"x_1412-05:BPMy:frmOff:val"
};                      
