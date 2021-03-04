// http://feelmare.blogspot.com/2014/04/video-stabilizing-example-source-code.html




    #define MAX_COUNT 250     
    #define DELAY_T 3  
    #define PI 3.1415     
      
      
    void main()     
    {     
      
     //////////////////////////////////////////////////////////////////////////     
     //image class           
     IplImage* image = 0;     
      
     //T, T-1 image     
     IplImage* current_Img = 0;     
     IplImage* Old_Img = 0;     
      
     //Optical Image     
     IplImage * imgA=0;     
     IplImage * imgB=0;     
      
      
     //Video Load     
     CvCapture * capture = cvCreateFileCapture("cam1.wmv"); //cvCaptureFromCAM(0); //cvCreateFileCapture("1.avi");     
      
     //Window     
     cvNamedWindow( "Origin OpticalFlow" , WINDOW_NORMAL);  
     //cvNamedWindow( "RealOrigin" , WINDOW_NORMAL);  
     //////////////////////////////////////////////////////////////////////////     
      
      
     //////////////////////////////////////////////////////////////////////////      
     //Optical Flow Variables      
     IplImage * eig_image=0;  
     IplImage * tmp_image=0;  
     int corner_count = MAX_COUNT;     
     CvPoint2D32f* cornersA = new CvPoint2D32f[ MAX_COUNT ];     
     CvPoint2D32f * cornersB = new CvPoint2D32f[ MAX_COUNT ];     
      
     CvSize img_sz;     
     int win_size=20;     
      
     IplImage* pyrA=0;     
     IplImage* pyrB=0;     
      
     char features_found[ MAX_COUNT ];     
     float feature_errors[ MAX_COUNT ];     
     //////////////////////////////////////////////////////////////////////////     
      
      
     //////////////////////////////////////////////////////////////////////////     
     //Variables for time different video     
     int one_zero=0;     
     //int t_delay=0;     
      
     double gH[9]={1,0,0, 0,1,0, 0,0,1};  
     CvMat gmxH = cvMat(3, 3, CV_64F, gH);  
      
      
      
     //Routine Start     
     while(1) {        
      
      
      //capture a frame form cam        
      if( cvGrabFrame( capture ) == 0 )     
       break;     
      //image = cvRetrieveFrame( capture );     
      //cvShowImage("RealOrigin", image );  
      
      
      //Image Create     
      if(Old_Img == 0)        
      {        
       image = cvRetrieveFrame( capture );     
       current_Img = cvCreateImage(cvSize(image->width, image->height), image->depth, image->nChannels);  
       memcpy(current_Img->imageData, image->imageData, sizeof(char)*image->imageSize );  
       Old_Img  = cvCreateImage(cvSize(image->width, image->height), image->depth, image->nChannels);  
       one_zero=1;  
      }     
      
      
      
      if(one_zero == 0 )     
      {     
       if(eig_image == 0)  
       {  
        eig_image = cvCreateImage(cvSize(image->width, image->height), image->depth, image->nChannels);  
        tmp_image = cvCreateImage(cvSize(image->width, image->height), image->depth, image->nChannels);  
       }  
      
       //copy to image class     
       memcpy(Old_Img->imageData, current_Img->imageData, sizeof(char)*image->imageSize );     
       image = cvRetrieveFrame( capture );     
       memcpy(current_Img->imageData, image->imageData, sizeof(char)*image->imageSize );     
      
       //////////////////////////////////////////////////////////////////////////     
       //Create image for Optical flow     
       if(imgA == 0)     
       {     
        imgA = cvCreateImage( cvSize(image->width, image->height), IPL_DEPTH_8U, 1);     
        imgB = cvCreateImage( cvSize(image->width, image->height), IPL_DEPTH_8U, 1);         
       }        
      
       //RGB to Gray for Optical Flow     
       cvCvtColor(current_Img, imgA, CV_BGR2GRAY);     
       cvCvtColor(Old_Img, imgB, CV_BGR2GRAY);        
      
       //extract features  
       cvGoodFeaturesToTrack(imgA, eig_image, tmp_image, cornersA, &corner_count, 0.01, 5.0, 0, 3, 0, 0.04);     
       cvFindCornerSubPix(imgA, cornersA, corner_count, cvSize(win_size, win_size), cvSize(-1, -1), cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03));        
      
      
       CvSize pyr_sz = cvSize( imgA->width+8, imgB->height/3 );     
       if( pyrA == 0)     
       {      
        pyrA = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1);     
        pyrB = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1);     
       }     
      
       //Optical flow  
       cvCalcOpticalFlowPyrLK( imgA, imgB, pyrA, pyrB, cornersA, cornersB, corner_count, cvSize(win_size, win_size), 5, features_found, feature_errors, cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3), 0);     
      
       /////////////////////////////////////////////////////////////////////////        
       int fCount=0;  
       for(int i=0; i < corner_count; ++i)  
       {  
      
        if( features_found[i] == 0 || feature_errors[i] > MAX_COUNT )  
         continue;  
      
        fCount++;  
        //////////////////////////////////////////////////////////////////////////         
        //Vector Length     
        //float fVecLength = sqrt((float)((cornersA[i].x-cornersB[i].x)*(cornersA[i].x-cornersB[i].x)+(cornersA[i].y-cornersB[i].y)*(cornersA[i].y-cornersB[i].y)));     
        //Vector Angle     
        //float fVecSetha  = fabs( atan2((float)(cornersB[i].y-cornersA[i].y), (float)(cornersB[i].x-cornersA[i].x)) * 180/PI );     
        //cvLine( image, cvPoint(cornersA[i].x, cornersA[i].y), cvPoint(cornersB[i].x, cornersA[i].y), CV_RGB(0, 255, 0), 2);      
       }  
      
       printf("%d \n", fCount);  
      
       int inI=0;  
       CvPoint2D32f* pt1 = new CvPoint2D32f[ fCount ];  
       CvPoint2D32f * pt2 = new CvPoint2D32f[ fCount ];  
       for(int i=0; i < corner_count; ++i)  
       {  
        if( features_found[i] == 0 || feature_errors[i] > MAX_COUNT )  
         continue;  
        pt1[inI] = cornersA[i];  
        pt2[inI] = cornersB[i];  
          
        cvLine( image, cvPoint(pt1[inI].x, pt1[inI].y), cvPoint(pt2[inI].x, pt2[inI].y), CV_RGB(0, 255, 0), 2);      
        inI++;  
       }  
      
       //FindHomography  
       CvMat M1, M2;  
       double H[9];  
       CvMat mxH = cvMat(3, 3, CV_64F, H);  
       M1 = cvMat(1, fCount, CV_32FC2, pt1);  
       M2 = cvMat(1, fCount, CV_32FC2, pt2);  
      
       //M2 = H*M1 , old = H*current  
       if( !cvFindHomography(&M1, &M2, &mxH, CV_RANSAC, 2))  //if( !cvFindHomography(&M1, &M2, &mxH, CV_RANSAC, 2))  
       {                   
        printf("Find Homography Fail!\n");  
          
       }else{  
        //printf(" %lf %lf %lf \n %lf %lf %lf \n %lf %lf %lf\n", H[0], H[1], H[2], H[3], H[4], H[5], H[6], H[7], H[8] );  
       }  
      
       delete pt1;  
       delete pt2;      
      
       //warping by H  
       //warpAffine(warped_2,warped_3,Transform_avg,Size( reSizeMat.cols, reSizeMat.rows));  
       //warpPerspective(cameraFrame2, WarpImg, H, Size(WarpImg.cols, WarpImg.rows));     
       IplImage* WarpImg =  cvCreateImage(cvSize(image->width, image->height), image->depth, image->nChannels);;  
         
       //cvCreateImage(cvSize(T1Img->width*2, T1Img->height*2), T1Img->depth, T1Img->nChannels);  
      
       cvMatMul( &gmxH, &mxH, &gmxH);   // Ma*Mb   -> Mc  
       printf(" %lf %lf %lf \n %lf %lf %lf \n %lf %lf %lf\n", H[0], H[1], H[2], H[3], H[4], H[5], H[6], H[7], H[8] );  
       printf(" -----\n");  
       printf(" %lf %lf %lf \n %lf %lf %lf \n %lf %lf %lf\n\n\n", gH[0], gH[1], gH[2], gH[3], gH[4], gH[5], gH[6], gH[7], gH[8] );  
         
       //cvWarpAffine(current_Img, WarpImg, &gmxH);  
       cvWarpPerspective(current_Img, WarpImg, &gmxH);   
       //cvWarpPerspective(Old_Img, WarpImg, &mxH);   
      
      
       //display  
       cvNamedWindow("Stabilizing",WINDOW_NORMAL );  
       cvShowImage("Stabilizing", WarpImg);   
      
       cvReleaseImage(&WarpImg);  
       //  
       //printf("[%d] - Sheta:%lf, Length:%lf\n",i , fVecSetha, fVecLength);     
      
      
      
       //cvWaitKey(0);  
       //////////////////////////////////////////////////////////////////////////         
      
      }     
      cvShowImage( "Origin OpticalFlow", image);     
      
      //////////////////////////////////////////////////////////////////////////     
      
      //time delay     
      one_zero++;  
      if( (one_zero % DELAY_T ) == 0)     
      {        
       one_zero=0;     
      }     
      
      //break        
      if( cvWaitKey(10) >= 0 )        
       break;        
     }        
      
     //release capture point        
     cvReleaseCapture(&capture);     
     //close the window        
     cvDestroyWindow( "Origin" );        
      
     cvReleaseImage(&Old_Img);      
     //////////////////////////////////////////////////////////////////////////     
     cvReleaseImage(&imgA);     
     cvReleaseImage(&imgB);      
     cvReleaseImage(&eig_image);  
     cvReleaseImage(&tmp_image);  
     delete cornersA;     
     delete cornersB;      
     cvReleaseImage(&pyrA);     
     cvReleaseImage(&pyrB);     
      
      
     //////////////////////////////////////////////////////////////////////////     
    }     
