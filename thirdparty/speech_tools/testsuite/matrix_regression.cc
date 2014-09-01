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

#include <cstdlib>
#include <iostream>
#include "EST_TMatrix.h"
#include "EST_String.h"


/**@name matrix_example
  * 
  * some stuff about matrices
  *
  * @see EST_TMatrix
  */
//@{

int main(void)
{
  EST_TMatrix<int> m(5,5);

  cout << "EST_TVector<int> size = " << sizeof(EST_TVector<int>) << " bytes.\n";
  cout << "EST_TMatrix<int> size = " << sizeof(EST_TMatrix<int>) << " bytes.\n";

  for(int i=0; i<m.num_rows(); i++)
    for(int j=0; j<m.num_columns(); j++)
      m.a(i,j) = i*100+j;

  cout << "Initial Matrix\n";
  m.save("-");
  cout << "\n";

  m.resize(m.num_rows(), m.num_columns());

  cout << "Same Sized Matrix\n";
  m.save("-");
  cout << "\n";

  m.resize(m.num_rows()*2, m.num_columns()*2);

  cout << "Double Sized Matrix\n";
  m.save("-");
  cout << "\n";

  for(int i2=0; i2<m.num_rows(); i2++)
    for(int j2=0; j2<m.num_columns(); j2++)
      m.a(i2,j2) = i2*1000+j2;

  cout << "Reset Matrix\n";
  m.save("-");
  cout << "\n";

  m.resize(m.num_rows()/2, m.num_columns()/2);

  cout << "Half Sized Matrix\n";
  m.save("-");
  cout << "\n";

  EST_TVector<int> v(5);

  for(int i7=0; i7<v.num_columns(); i7++)
    v[i7] = i7;

  cout << v << "\n";

  v.resize(5);

  cout << v << "\n";

  v.resize(10);

  cout << v << "\n";

  for(int i8=0; i8<v.num_columns(); i8++)
    v[i8] = 500 + i8;

  cout << v << "\n";

  v.resize(5, 0);

  return(0);
}

//@}

