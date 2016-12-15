// Copyright Daniel J. Steffey -- 2016

#include "GShaderCompose.hpp"
#include "utils.hpp"
#include <iostream>

GShaderCompose::GShaderCompose(GShader* shader_1, GShader* shader_2)
{
	// save the shader
	this->m_shader_1 = shader_1;
	this->m_shader_2 = shader_2;
}

GShaderCompose::~GShaderCompose()
{
	// nothing to destroy
}

bool GShaderCompose::setContext(const GMatrix& ctm, float alpha)
{
	// forward the setContext
	if (this->m_shader_1->setContext(ctm, alpha) == false)
	{
		return false;
	}
	if (this->m_shader_2->setContext(ctm, alpha) == false)
	{
		return false;
	}

	// success
	return true;
}

void GShaderCompose::shadeRow(int x, int y, int count, GPixel row[])
{
	// have each shader operate their own shade row
	GPixel temp_row[256];

	this->m_shader_1->shadeRow(x, y, count, row);
	this->m_shader_2->shadeRow(x, y, count, temp_row);

	for (int i = 0; i < count; ++i)
	{
		int a0 = GPixel_GetA(row[i]);
		int r0 = GPixel_GetR(row[i]);
		int g0 = GPixel_GetG(row[i]);
		int b0 = GPixel_GetB(row[i]);

		int a1 = GPixel_GetA(temp_row[i]);
		int r1 = GPixel_GetR(temp_row[i]);
		int g1 = GPixel_GetG(temp_row[i]);
		int b1 = GPixel_GetB(temp_row[i]);

		int a2 = divide_by_255(a0 * a1);
		int r2 = divide_by_255(r0 * r1);
		int g2 = divide_by_255(g0 * g1);
		int b2 = divide_by_255(b0 * b1);

		row[i] = GPixel_PackARGB(a2, r2, g2, b2);
	}
}