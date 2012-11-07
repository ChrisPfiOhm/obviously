#include "MatRGB.h"

#include <gsl/gsl_vector_uchar.h>
#include <gsl/gsl_matrix_uchar.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_permutation.h>
#include <iostream>

namespace {
const unsigned int CHANNELS = 3;

#define GSL(x) (static_cast<gsl_matrix_uchar*>(x))
}

namespace obvious {

MatRGB::MatRGB(const unsigned int rows, const unsigned int cols)
    : AbstractMat(rows, cols)
{
    if (!_rows && !_cols)
        return;

    for (unsigned int i = 0; i < CHANNELS; i++)
        _data.push_back(gsl_matrix_uchar_alloc(_rows, _cols));
}

MatRGB::MatRGB(const MatRGB& mat)
    : AbstractMat(mat._rows, mat._cols)
{
    if (!_rows && !_cols)
        return;

    for (unsigned int i = 0; i < CHANNELS; i++)
    {
        _data.push_back(gsl_matrix_uchar_alloc(_rows, _cols));
        gsl_matrix_uchar_memcpy(GSL(_data[i]), GSL(mat._data[i]));
    }
}

MatRGB::MatRGB(MatRGB& mat)
    : AbstractMat(mat._rows, mat._cols)
{
    AbstractMat<unsigned char>::operator=(mat);
}

MatRGB::~MatRGB(void)
{
    /* Check if m_data has to be deleted */
    if (this->haveToFreeData())
        for (unsigned int i = 0; i < _data.size(); i++)
            gsl_matrix_uchar_free(GSL(_data[i]));
}

unsigned char& MatRGB::at(const unsigned int row, const unsigned int col, const unsigned int channel)
{
    return *gsl_matrix_uchar_ptr(GSL(_data[channel]), row, col);
}

unsigned char MatRGB::at(const unsigned int row, const unsigned int col, const unsigned int channel) const
{
    return gsl_matrix_uchar_get(GSL(_data[channel]), row, col);
}

RGBColor MatRGB::rgb(const unsigned int row, const unsigned int col) const
{
    return RGBColor(gsl_matrix_uchar_get(GSL(_data[Red])  , row, col),
                    gsl_matrix_uchar_get(GSL(_data[Green]), row, col),
                    gsl_matrix_uchar_get(GSL(_data[Blue]) , row, col));
}

MatRGB& MatRGB::operator=(MatRGB& mat)
{
    /* Before take a reference to another Mat, delete m_data */
    if (this->haveToFreeData())
        for (unsigned int i = 0; i < _data.size(); i++)
            gsl_matrix_uchar_free(GSL(_data[i]));

    AbstractMat<unsigned char>::operator=(mat);

    return *this;
}

MatRGB& MatRGB::operator=(MatRGB mat)
{
    /* Before take a reference to another Mat, delete m_data */
    if (this->haveToFreeData())
        for (unsigned int i = 0; i < _data.size(); i++)
            gsl_matrix_uchar_free(GSL(_data[i]));

    AbstractMat<unsigned char>::operator=(mat);

    return *this;
}

}
