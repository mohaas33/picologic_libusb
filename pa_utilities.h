#ifndef __PA_UTILITIES_H__
#define __PA_UTILITIES_H__

#include <vector>

void readCalibration(double *amp, double *bias);
int pa_connect();
std::string communicate_pAs_v(double *amp, double *bias, int BUFFER_SIZE, int format);

#endif // __PA_UTILITIES_H__
