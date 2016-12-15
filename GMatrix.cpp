// Copyright Daniel J. Steffey -- 2016

#include "include/GMatrix.h"

void GMatrix::setIdentity()
{
	this->fMat[SX] = 1;
	this->fMat[KX] = 0;
	this->fMat[TX] = 0;
	this->fMat[KY] = 0;
	this->fMat[SY] = 1;
	this->fMat[TY] = 0;
}

void GMatrix::setTranslate(float tx, float ty)
{
   	this->fMat[SX] = 1;
	this->fMat[KX] = 0;
	this->fMat[TX] = tx;
	this->fMat[KY] = 0;
	this->fMat[SY] = 1;
	this->fMat[TY] = ty;
}

void GMatrix::setScale(float sx, float sy)
{
   	this->fMat[SX] = sx;
	this->fMat[KX] = 0;
	this->fMat[TX] = 0;
	this->fMat[KY] = 0;
	this->fMat[SY] = sy;
	this->fMat[TY] = 0;
}

void GMatrix::setRotate(float radians)
{
    this->fMat[SX] = std::cos(radians);
    this->fMat[KX] = -(std::sin(radians));
    this->fMat[TX] = 0;
    this->fMat[KY] = std::sin(radians);
    this->fMat[SY] = std::cos(radians);
    this->fMat[TY] = 0;
}

void GMatrix::setConcat(const GMatrix& secundo, const GMatrix& primo)
{
    float sx = (secundo.fMat[SX] * primo.fMat[SX]) + (secundo.fMat[KX] * primo.fMat[KY]);
    float kx = (secundo.fMat[SX] * primo.fMat[KX]) + (secundo.fMat[KX] * primo.fMat[SY]);
    float tx = (secundo.fMat[SX] * primo.fMat[TX]) + (secundo.fMat[KX] * primo.fMat[TY]) + secundo.fMat[TX];
    float ky = (secundo.fMat[KY] * primo.fMat[SX]) + (secundo.fMat[SY] * primo.fMat[KY]);
    float sy = (secundo.fMat[KY] * primo.fMat[KX]) + (secundo.fMat[SY] * primo.fMat[SY]);
    float ty = (secundo.fMat[KY] * primo.fMat[TX]) + (secundo.fMat[SY] * primo.fMat[TY]) + secundo.fMat[TY];

    this->fMat[SX] = sx;
    this->fMat[KX] = kx;
    this->fMat[TX] = tx;
    this->fMat[KY] = ky;
    this->fMat[SY] = sy;
    this->fMat[TY] = ty;
}

bool GMatrix::invert(GMatrix* inverse) const
{
	// calculate the determinate
	float determinate = (this->fMat[SX] * this->fMat[SY]) - (this->fMat[KX] * this->fMat[KY]);

	// check for valid determinate
	if (determinate == 0)
	{
		// cannot calculate the inverse
		return false;
	}

	// has an inverse but no parameter to store it in
	if (inverse == nullptr)
	{
		return true;
	}

	// store the value
	float sx = this->fMat[SY] / determinate;
	float kx = -(this->fMat[KX]) / determinate;
	float tx = ((this->fMat[KX] * this->fMat[TY]) - (this->fMat[SY] * this->fMat[TX])) / determinate;
	float ky = -(this->fMat[KY]) / determinate;
	float sy = this->fMat[SX] / determinate;
	float ty = ((this->fMat[KY] * this->fMat[TX]) - (this->fMat[SX] * this->fMat[TY])) / determinate;

	inverse->fMat[SX] = sx;
	inverse->fMat[KX] = kx;
	inverse->fMat[TX] = tx;
	inverse->fMat[KY] = ky;
	inverse->fMat[SY] = sy;
	inverse->fMat[TY] = ty;

    return true;
}

void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const
{
	for (int i = 0; i < count; ++i)
	{
        GPoint temp;

		temp.fX = (this->fMat[SX] * src[i].fX) + (this->fMat[KX] * src[i].fY) + this->fMat[TX];
		temp.fY = (this->fMat[KY] * src[i].fX) + (this->fMat[SY] * src[i].fY) + this->fMat[TY];
	
		dst[i].fX = temp.fX;
		dst[i].fY = temp.fY;
	}
}