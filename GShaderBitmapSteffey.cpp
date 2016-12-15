// Copyright Daniel J. Steffey -- 2016

#include "GShaderBitmapSteffey.hpp"


GShader* GShader::FromBitmap(const GBitmap& bitmap, const GMatrix& local_matrix, GShader::TileMode tilemode)
{
	// check for valid parameters on the bitmap
	if (bitmap.fWidth <= 0)
	{
		return nullptr;
	}
	if (bitmap.fHeight <= 0)
	{
		return nullptr;
	}
	if (bitmap.fPixels == nullptr)
	{
		return nullptr;
	}
	if (bitmap.fRowBytes < (bitmap.fWidth * 4))
	{
		return nullptr;
	}
	// the local matrix can really be about anything
	// so no checks there
	// return out shader
	return new GShaderBitmapSteffey(bitmap, local_matrix, tilemode);
}

GShaderBitmapSteffey::GShaderBitmapSteffey(const GBitmap& bitmap, const GMatrix& local_ctm, GShader::TileMode tilemode)
{
	this->m_bitmap = &bitmap;
	this->m_local_ctm = local_ctm;
	this->m_global_ctm.setIdentity();
	this->m_combined_ctm.setConcat(this->m_local_ctm, this->m_global_ctm);
	this->m_combined_ctm.invert(&(this->m_combined_ctm));
	this->m_alpha = 1.0f;
	this->m_tilemode = tilemode;
}

GShaderBitmapSteffey::~GShaderBitmapSteffey()
{
	// nothing to destroy
}

bool GShaderBitmapSteffey::setContext(const GMatrix& ctm, float alpha)
{
	// save the passed in global ctm
	this->m_global_ctm = ctm;

	// save the alpha
	this->m_alpha = alpha;

	// recompute our combined and invert it
	this->m_combined_ctm.setConcat(this->m_global_ctm, this->m_local_ctm);
	if (this->m_combined_ctm.invert(&(this->m_combined_ctm)) == false)
	{
		return false;
	}
	
	// success
	return true;
}

void GShaderBitmapSteffey::shadeRow(int x, int y, int count, GPixel row[])
{
	// calculate the first source point
	GPoint src_point = this->m_combined_ctm.mapXY(x + 0.5f, y + 0.5f);

	// loop through the count number of pixels
	for (int i = 0; i < count; ++i)
	{
		int src_x = 0;
		int src_y = 0;
		if (this->m_tilemode == GShader::kClamp)
		{
			// clamp to the min (0) and max (bitmap width - 1) values
			src_x = std::max(0, std::min((int)(src_point.fX), this->m_bitmap->fWidth - 1));
			src_y = std::max(0, std::min((int)(src_point.fY), this->m_bitmap->fHeight - 1));
		}
		else if (this->m_tilemode == GShader::kRepeat)
		{
			src_x = ((int)(src_point.fX)) % (this->m_bitmap->fWidth);
			src_y = ((int)(src_point.fY)) % (this->m_bitmap->fHeight);
		}
		else if (this->m_tilemode == GShader::kMirror)
		{
			src_x = ((int)(src_point.fX)) % (this->m_bitmap->fWidth * 2);
			src_y = ((int)(src_point.fY)) % (this->m_bitmap->fHeight * 2);
			if (src_x >= this->m_bitmap->fWidth)
			{
				src_x = (this->m_bitmap->fWidth * 2) - src_x - 1;
			}
			if (src_y >= this->m_bitmap->fHeight)
			{
				src_y = (this->m_bitmap->fHeight * 2) - src_y - 1;
			}
		}

		// get the row of pixels from the source
		GPixel* src_row = this->m_bitmap->pixels() + ((this->m_bitmap->rowBytes() >> 2) * src_y);

		// now need to modulate that by the context alpha
		if (this->m_alpha == 1.0f)
		{
			row[i] = src_row[src_x];
		}
		else
		{
			int a = GPixel_GetA(src_row[src_x]) * this->m_alpha;
			int r = GPixel_GetR(src_row[src_x]) * this->m_alpha;
			int g = GPixel_GetG(src_row[src_x]) * this->m_alpha;
			int b = GPixel_GetB(src_row[src_x]) * this->m_alpha;

			// save it to the row
			row[i] = GPixel_PackARGB(a, r, g, b);
		}

		// increment the source point based on values in the matrix
		src_point.fX += this->m_combined_ctm[GMatrix::SX];
		src_point.fY += this->m_combined_ctm[GMatrix::KY];

	}
}