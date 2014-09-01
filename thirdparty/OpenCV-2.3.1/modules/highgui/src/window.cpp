/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"

// in later times, use this file as a dispatcher to implementations like cvcap.cpp

CV_IMPL void cvSetWindowProperty(const char* name, int prop_id, double prop_value)
{
	switch(prop_id)
	{
		//change between fullscreen or not.
		case CV_WND_PROP_FULLSCREEN:
		
			if (!name || (prop_value!=CV_WINDOW_NORMAL && prop_value!=CV_WINDOW_FULLSCREEN))//bad argument
				break;
		
			#if defined (HAVE_QT)
			cvSetModeWindow_QT(name,prop_value);
			#elif defined WIN32 || defined _WIN32 
			cvSetModeWindow_W32(name,prop_value);
			#elif defined (HAVE_GTK)
			cvSetModeWindow_GTK(name,prop_value);
			#elif defined (HAVE_CARBON)
			cvSetModeWindow_CARBON(name,prop_value);
			#endif
		break;
		
		case CV_WND_PROP_AUTOSIZE:
			#if defined (HAVE_QT)
				cvSetPropWindow_QT(name,prop_value);
			#endif
		break;
		
		case CV_WND_PROP_ASPECTRATIO:
			#if defined (HAVE_QT)
				cvSetRatioWindow_QT(name,prop_value);
			#endif
		break;
		
	default:;
	}
}

/* return -1 if error */
CV_IMPL double cvGetWindowProperty(const char* name, int prop_id)
{
	switch(prop_id)
	{
		case CV_WND_PROP_FULLSCREEN:
		
			if (!name)//bad argument
				return -1;
			
            #if defined (HAVE_QT)
				return cvGetModeWindow_QT(name);
            #elif defined WIN32 || defined _WIN32 
				return cvGetModeWindow_W32(name);
			#elif defined (HAVE_GTK)
				return cvGetModeWindow_GTK(name);
			#elif defined (HAVE_CARBON)
				return cvGetModeWindow_CARBON(name);
			#else
                return -1;
			#endif
		
		case CV_WND_PROP_AUTOSIZE:
		
			if (!name)//bad argument
				return -1;
				
			#if defined (HAVE_QT)
				return cvGetPropWindow_QT(name);
			#else
				return -1;
		    #endif	
		    
		case CV_WND_PROP_ASPECTRATIO:
			#if defined (HAVE_QT)
				return cvGetRatioWindow_QT(name);
			#else
				return -1;
		    #endif	
		break;
		
	    default:
		    return -1;
	}
}

void cv::namedWindow( const string& winname, int flags )
{
    cvNamedWindow( winname.c_str(), flags );
}

void cv::destroyWindow( const string& winname )
{
    cvDestroyWindow( winname.c_str() );
}

void cv::destroyAllWindows()
{
    cvDestroyAllWindows();
}

void cv::setWindowProperty(const string& winname, int prop_id, double prop_value)
{
	cvSetWindowProperty( winname.c_str(),prop_id,prop_value);
}

double cv::getWindowProperty(const string& winname, int prop_id)
{
	return  cvGetWindowProperty(winname.c_str(),prop_id);
}

void cv::imshow( const string& winname, InputArray _img )
{
    Mat img = _img.getMat();
    CvMat c_img = img;
    cvShowImage( winname.c_str(), &c_img );
}

int cv::waitKey(int delay)
{
    return cvWaitKey(delay);
}

int cv::createTrackbar(const string& trackbarName, const string& winName,
                   int* value, int count, TrackbarCallback callback,
                   void* userdata)
{
    return cvCreateTrackbar2(trackbarName.c_str(), winName.c_str(),
                             value, count, callback, userdata);
}

void cv::setTrackbarPos( const string& trackbarName, const string& winName, int value )
{
    cvSetTrackbarPos(trackbarName.c_str(), winName.c_str(), value );
}

int cv::getTrackbarPos( const string& trackbarName, const string& winName )
{
	return cvGetTrackbarPos(trackbarName.c_str(), winName.c_str());
}
    
void cv::setMouseCallback( const string& windowName, MouseCallback onMouse, void* param)
{
    cvSetMouseCallback(windowName.c_str(), onMouse, param);
}
    
int cv::startWindowThread()
{
    return cvStartWindowThread();
}

#if defined (HAVE_QT)

CvFont cv::fontQt(const string& nameFont, int pointSize, Scalar color, int weight,  int style, int spacing)
{
return cvFontQt(nameFont.c_str(), pointSize,color,weight, style);
}

void cv::addText( const Mat& img, const string& text, Point org, CvFont font)
{
	CvMat _img = img;
	cvAddText( &_img, text.c_str(), org,&font);
}

void cv::displayStatusBar(const string& name,  const string& text, int delayms)
{
	cvDisplayStatusBar(name.c_str(),text.c_str(), delayms);
}

void cv::createOpenGLCallback(const string& name,  OpenGLCallback callback, void* param)
{
	cvCreateOpenGLCallback(name.c_str(),callback, param);
}

void cv::displayOverlay(const string& name,  const string& text, int delayms)
{
	cvDisplayOverlay(name.c_str(),text.c_str(), delayms);
}

int cv::startLoop(int (*pt2Func)(int argc, char *argv[]), int argc, char* argv[])
{
	return cvStartLoop(pt2Func, argc, argv);
}

void cv::stopLoop()
{
	cvStopLoop();
}

void cv::saveWindowParameters(const string& windowName)
{
	cvSaveWindowParameters(windowName.c_str());
}

void cv::loadWindowParameters(const string& windowName)
{
	cvLoadWindowParameters(windowName.c_str());
}

int cv::createButton(const string& button_name, ButtonCallback on_change, void* userdata, int button_type , bool initial_button_state  )
{
	return cvCreateButton(button_name.c_str(), on_change, userdata, button_type , initial_button_state );
}

#endif

#if   defined WIN32 || defined _WIN32         // see window_w32.cpp
#elif defined (HAVE_GTK)      // see window_gtk.cpp
#elif defined (HAVE_COCOA)   // see window_carbon.cpp
#elif defined (HAVE_CARBON)
#elif defined (HAVE_QT) //YV see window_QT.cpp

#else

// No windowing system present at compile time ;-(
// 
// We will build place holders that don't break the API but give an error
// at runtime. This way people can choose to replace an installed HighGUI
// version with a more capable one without a need to recompile dependent
// applications or libraries.


#define CV_NO_GUI_ERROR(funcname) \
    cvError( CV_StsError, funcname, \
    "The function is not implemented. " \
    "Rebuild the library with Windows, GTK+ 2.x or Carbon support. "\
    "If you are on Ubuntu or Debian, install libgtk2.0-dev and pkg-config, then re-run cmake or configure script", \
    __FILE__, __LINE__ )


CV_IMPL int cvNamedWindow( const char*, int )
{
    CV_NO_GUI_ERROR("cvNamedWindow");
    return -1;
}     

CV_IMPL void cvDestroyWindow( const char* )
{
    CV_NO_GUI_ERROR( "cvDestroyWindow" );
}

CV_IMPL void
cvDestroyAllWindows( void )
{
    CV_NO_GUI_ERROR( "cvDestroyAllWindows" );
}

CV_IMPL void
cvShowImage( const char*, const CvArr* )
{
    CV_NO_GUI_ERROR( "cvShowImage" );
}

CV_IMPL void cvResizeWindow( const char*, int, int )
{
    CV_NO_GUI_ERROR( "cvResizeWindow" );
}

CV_IMPL void cvMoveWindow( const char*, int, int )
{
    CV_NO_GUI_ERROR( "cvMoveWindow" );
}

CV_IMPL int
cvCreateTrackbar( const char*, const char*,
                  int*, int, CvTrackbarCallback )
{
    CV_NO_GUI_ERROR( "cvCreateTrackbar" );
    return -1;
}

CV_IMPL int
cvCreateTrackbar2( const char* trackbar_name, const char* window_name,
                   int* val, int count, CvTrackbarCallback2 on_notify2,
                   void* userdata )
{
    CV_NO_GUI_ERROR( "cvCreateTrackbar2" );
    return -1;
}

CV_IMPL void
cvSetMouseCallback( const char*, CvMouseCallback, void* )
{
    CV_NO_GUI_ERROR( "cvSetMouseCallback" );
}

CV_IMPL int cvGetTrackbarPos( const char*, const char* )
{
    CV_NO_GUI_ERROR( "cvGetTrackbarPos" );
    return -1;
}

CV_IMPL void cvSetTrackbarPos( const char*, const char*, int )
{
    CV_NO_GUI_ERROR( "cvSetTrackbarPos" );
}

CV_IMPL void* cvGetWindowHandle( const char* )
{
    CV_NO_GUI_ERROR( "cvGetWindowHandle" );
    return 0;
}
    
CV_IMPL const char* cvGetWindowName( void* )
{
    CV_NO_GUI_ERROR( "cvGetWindowName" );
    return 0;
}

CV_IMPL int cvWaitKey( int )
{
    CV_NO_GUI_ERROR( "cvWaitKey" );
    return -1;
}

CV_IMPL int cvInitSystem( int argc, char** argv )
{

    CV_NO_GUI_ERROR( "cvInitSystem" );
    return -1;
}

CV_IMPL int cvStartWindowThread()
{

    CV_NO_GUI_ERROR( "cvStartWindowThread" );
    return -1;
}


//-------- Qt ---------
CV_IMPL void cvAddText( const CvArr*, const char*, CvPoint org, CvFont* font)
{
    CV_NO_GUI_ERROR("cvAddText");
}

CV_IMPL void cvDisplayStatusBar(const char* name, const char* arg2, int arg3)
{
    CV_NO_GUI_ERROR("cvDisplayStatusBar");
}  

CV_IMPL void cvCreateOpenGLCallback(const char*, void (*)(void*), void*, double, double, double)
{
    CV_NO_GUI_ERROR("createOpenGLCallback");
}  

CV_IMPL void cvDisplayOverlay(const char* name, const char* text, int delayms)
{
    CV_NO_GUI_ERROR("cvNamedWindow");
}  

CV_IMPL int cvStartLoop(int (*pt2Func)(int argc, char *argv[]), int argc, char* argv[])
{
    CV_NO_GUI_ERROR("cvStartLoop");
    return -1;
}  

CV_IMPL void cvStopLoop()
{
    CV_NO_GUI_ERROR("cvStopLoop");
}  

CV_IMPL void cvSaveWindowParameters(const char* name)
{
    CV_NO_GUI_ERROR("cvSaveWindowParameters");
}  

CV_IMPL void cvLoadWindowParameterss(const char* name)
{
    CV_NO_GUI_ERROR("cvLoadWindowParameters");
}  

CV_IMPL int cvCreateButton(const char*, void (*)(int, void*), void*, int, int)
{
    CV_NO_GUI_ERROR("cvCreateButton");
    return -1;
}  


#endif

/* End of file. */
