#include <math.h>
#include "Vector.h"

float DegreesToRadians(float degrees)
{
	return (float)(degrees * M_PI / 180.0f);
}

double Get3dDistance(Vector myCoords, Vector enemyCoords)
{
	return sqrt(
		pow(double(enemyCoords.X - myCoords.X), 2.0) +
		pow(double(enemyCoords.Y - myCoords.Y), 2.0) +
		pow(double(enemyCoords.Z - myCoords.Z), 2.0));

}

void GetRandomAlphanumeric(char *s, const int len)
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i)
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

    s[len] = 0;
}
