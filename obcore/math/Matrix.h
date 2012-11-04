#ifndef MATRIX_H__
#define MATRIX_H__

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>

#include <iostream>

using namespace std;

/**
 * @namespace obvious
 */
namespace obvious
{

/**
 * @class Matrix
 * @brief Matrix abstraction layer of GSL
 * @author Stefan May
 */
class Matrix
{
public:
	/**
	 * Constructor
	 * @param rows number of matrix rows
	 * @param cols number of matrix columns
	 */
	Matrix(unsigned int rows, unsigned int cols);

	/**
	 * Copy constructor
	 * @param M matrix to be copied
	 */
	Matrix(const Matrix &M);

	/**
	 * Destructor
	 */
	~Matrix();

	/**
	 * Assignment operator
	 * @param M matrix assigned to this one
	 * @return this matrix instance
	 */
	Matrix  &operator =  (const Matrix &M);

	/**
	 * Assignment operator
	 * @param M matrix assigned to this one
	 * @return this matrix instance
	 */
	Matrix  &operator *= (const Matrix &M);

	/**
	 * Row accessor
	 * @param i row index
	 * @return row elements as array
	 */
	double* operator [] (unsigned int i);

	/**
	 * Multiplication operator
	 * @param M1 1st matrix of product
	 * @param M2 2nd matrix of product
	 * @return matrix product
	 */
	friend Matrix operator * (const Matrix &M1, const Matrix &M2);

	/**
	 * Stream operator
	 * @param os output stream
	 * @param M matrix to be streamed, e.g. printed out
	 */
	friend ostream& operator <<(ostream &os, Matrix &M);

	/**
	 * GSL matrix accessor
	 * @brief get access to internal matrix representation
	 * @return GSL matrix
	 */
	gsl_matrix* getBuffer();

	/**
	 * Data accessor
	 * @param array array to copy data into (must be instanciated outside)
	 */
	void getData(double* array);

	/**
	 * Data mutator
	 * @param array array to copy data from
	 */
	void setData(double* array);

	/**
	 * Property accessor
	 * @return number of matrix rows
	 */
	unsigned int getRows();

	/**
	 * Property accessor
	 * @return number of matrix columns
	 */
	unsigned int getCols();

	/**
	 * Set matrix to identity
	 */
	void setIdentity();

	/**
	 * Set all matrix elements to zero
	 */
	void setZero();

	/**
	 * Instantiate an inverse of the present matrix
	 * @return inverse matrix as new instance
	 */
	Matrix getInverse();

	/**
	 * Invert present matrix
	 */
	void invert();

	/**
	 * Calculate trace of matrix
	 * @return trace
	 */
	double trace();

	/**
	 * Calculate centroid of matrix
	 * @return c-dimensional centroid vector, where c is the number of matrix columns
	 */
	gsl_vector* centroid();

	/**
	 * perform principle component analysis
	 * @return matrix in layout [x1_from x1_to y1_from y1_to z1_from z1_to; x2...]
	 */
	Matrix* pcaAnalysis();

	/**
	 * Instantiate a 4x4 translation matrix, i.e. identity with last column set to translational input
	 * @param tx x-component of translation
	 * @param ty y-component of translation
	 * @param tz z-component of translation
	 */
	static Matrix* TranslationMatrix44(double tx, double ty, double tz);

	/**
	 * Print matrix to output stream
	 */
	void print();

private:

	/**
	 * Internal GSL representation
	 */
	gsl_matrix* _M;

	/**
	 * Internal GSL work buffer
	 */
	gsl_matrix* _work;
};

}

#endif //MATRIX_H
