 /*************************************************************************/
 /*                                                                       */
 /*                Centre for Speech Technology Research                  */
 /*                     University of Edinburgh, UK                       */
 /*                       Copyright (c) 1996,1997                         */
 /*                        All Rights Reserved.                           */
 /*  Permission is hereby granted, free of charge, to use and distribute  */
 /*  this software and its documentation without restriction, including   */
 /*  without limitation the rights to use, copy, modify, merge, publish,  */
 /*  distribute, sublicense, and/or sell copies of this work, and to      */
 /*  permit persons to whom this work is furnished to do so, subject to   */
 /*  the following conditions:                                            */
 /*   1. The code must retain the above copyright notice, this list of    */
 /*      conditions and the following disclaimer.                         */
 /*   2. Any modifications must be clearly marked as such.                */
 /*   3. Original authors' names are not deleted.                         */
 /*   4. The authors' names are not used to endorse or promote products   */
 /*      derived from this software without specific prior written        */
 /*      permission.                                                      */
 /*  THE UNIVERSITY OF EDINBURGH AND THE CONTRIBUTORS TO THIS WORK        */
 /*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
 /*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
 /*  SHALL THE UNIVERSITY OF EDINBURGH NOR THE CONTRIBUTORS BE LIABLE     */
 /*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
 /*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
 /*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
 /*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
 /*  THIS SOFTWARE.                                                       */
 /*                                                                       */
 /*************************************************************************/
 /*                                                                       */
 /*                 Author: Richard Caley (rjc@cstr.ed.ac.uk)             */
 /*                   Date: Tue Jul 22 1997                               */
 /* --------------------------------------------------------------------- */
 /* Example of matrix class use.                                          */
 /*                                                                       */
 /*************************************************************************/

/**@name EST_TMatrix 
  * 
  * The EST_TMatrix class is a general purpose 2 dimensional array
  * container class. It handles memory allocation and (if required) bounds
  * checking and is reasonably efficient, so there should be little need
  * to use bare &cpp; arrays.
  *
  * @see EST_TMatrix
  * @see EST_TVector
  */
//@{

#include <cstdlib>
#include <iostream>

//@{ code

#include "EST_TMatrix.h"
#include "EST_String.h"

EST_write_status save(const EST_String &filename, const EST_TVector<float> &a);
EST_write_status save(const EST_String &filename, const EST_TMatrix<float> &a);

//@} code

static inline int max(int a, int b) { return a>b?a:b; }


int main(void)
{
  /** @name Basic Matrix Use
    * Instances of the TMatrix class are intended to behave as
    * you would expect two dimensional arrays to work.
    */
    
  //@{
  /**@name Declaration
    * 
    * Matrices are declared by giving the type and the number of 
    * rows and columns. Here we create a 10 row by 5 column matrix.
    */
  //@{ code

  EST_TMatrix<float> m(10, 5);

  //@} code


  /**@name Access
    *
    * Access to values in the matrix is via the a() member function.
    * This returns a reference, so you can assign values to matrix cells.
    * As is usually the case in &cpp;, column and row indices start from 0.
    */
  //@{ code

  for(int i=0; i<m.num_rows(); i++)
    for(int j=0; j<m.num_columns(); j++)
      m.a(i,j) = i+j/100.0;	// Just something easy to recognise

  //@} code

  /**@name Output
    * A simple output method is supplied, it just outputs a row at a time,
    * tab separated to a named file. A filename of "-" means standard output.
    */
  //@{ code
  cout << "Initial Matrix\n";
  save("-",m);
  cout << "\n";
  //@} code

  /**@name Resizing
    * Resize to 20 rows by 10 columns This fills the new
    * area with <parameter>def_val</parameter>, which is 0.0 for floats.
    */
  //@{ code
  m.resize(20,10);
  //@} code

  cout << "Resized Matrix\n";
  save("-",m);
  cout << "\n";

  // Fill it with something easy to recognise.
  for(int i0=0; i0<m.num_rows(); i0++)
    for(int j=0; j<m.num_columns(); j++)
      m.a(i0,j) = i0+j/100.0;

  // Write to standard output in an ascii format.
  cout << "Full Matrix\n";
  save("-",m);
  cout << "\n";

  //@}

  /**@name Copying Data to/from a buffer
    * 
    * Whole rows or columns can be extracted into a buffer, or can be
    * filled with data from a buffer. The buffer must be pre-declared,
    * and it is up to you to ensure it is big enough.
    */
  //@{

  /** 
    * Data can be extracted into a buffer in one operation
    */
  //@{ code
  float *buf = new float[max(m.num_rows(),m.num_columns())];

  m.copy_row(5, buf);
  //@} code

  cout << "Row 5\n";
  for(int j1=0; j1<m.num_columns(); j1++)
    cout << buf[j1] << "\t";
  cout << "\n\n";

  /**
    * And data can be inserted in a similar manner.
    */
  for(int i1=0; i1<m.num_rows(); i1++)
    buf[i1] = i1+100;

  //@{ code

  m.set_column(5,buf);

  //@} code

  delete [] buf;

  //@}

  cout << "Updated Matrix (column 5 replaced with 100s from buffer)\n";
  save("-",m);
  cout << "\n";

  /**@name Sub-Matrices and Sub-Vectors
    *
    * A sub-vector or sub-matrix is a window onto a matrix. If you obtain a
    * sub vector representing a row, for instance, you can treat it
    * a normal vector, any changes you make affecting the underlying
    * matrix.
    */

  //@{

  /** Here is how we can create new variables which refer to the 11th
    * row, 4th column and a 5X3 rectangle with top left hand corner (8,2).
    * (since the first column or row is numbered 0, the numbers may be one
    * less than you expect).
    */

  //@{ code
  EST_TVector<float> row;
  EST_TVector<float> column;
  EST_TMatrix<float> rectangle;
  
  m.row(row, 10);
  m.column(column, 3);
  m.sub_matrix(rectangle, 
	       8, 5, 
	       2, 3);
  //@} code

  cout <<"Row 10 extracted as sub vector\n";
  save("-",row);
  cout << "\n";

  cout <<"Column 3 extracted as sub vector\n";
  save("-",column);
  cout << "\n";

  cout <<"Rectangle extracted as sub vector\n";
  save("-",rectangle);
  cout << "\n";

  /**  If we update the sub-vector, the main matrix changes.
    */

  //@{ code

  // 10th row becomes squares of the index
  for(int i2=0; i2<row.n(); i2++)
    row[i2] = i2*i2;

  // 3rd column becomes cubes of the index
  for(int i3=0; i3<column.n(); i3++)
    column[i3] = i3*i3*i3;

  // Central rectangle filled with -1
  for(int i4=0; i4<rectangle.num_rows(); i4++)
    for(int j4=0; j4<rectangle.num_columns(); j4++)
      rectangle.a(i4, j4) = -1;

  //@} code

  /** We can even extract rows and columns from a sub-matrix as follows.
    */

  //@{ code
  EST_TVector<float> rrow;
  EST_TVector<float> rcolumn;

  // 3rd row of sub-matrix, part of 12th row of main matrix
  rectangle.row(rrow, 2);
  // 2nd column of sub-matrix, part of 8th column of main matrix
  rectangle.column(rcolumn, 1);

  //@} code

  for(int i6=0; i6<rcolumn.n(); i6++)
    rcolumn[i6] = -3;
  for(int i5=0; i5<rrow.n(); i5++)
    rrow[i5] = -2;

  //@}

  cout << "Updated Matrix (row 10 becomes squares, column 3 becomes cubes, center becomes negative)\n";
  save("-",m);
  cout << "\n";

  exit(0);
}

/**@name Template Instantiation.
  * 
  * Some &cpp; compilers require explicit guidance about which types
  * of Matrix will be used. For many common types, including float,
  * this guidance is included in the &est; libraries. However, if you
  * need to use matrices of your own types, you will need to include
  * declarations similar to the following.
  */
//@{

#ifdef __NOT_REAL_CODE__

//@{ code

Declare_TMatrix(MyType)

#if defined(INSTANTIATE_TEMPLATES)

#include "../base_class/EST_TMatrix.cc"

Instantiate_TMatrix(MyType)

#endif

  //@} code

  /** By using this form of declaration, you should ensure that your
    * code will compile anywhere where the speech tools libraries will.
    */

 #endif

  //@}

//@}

