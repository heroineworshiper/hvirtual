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
// Copyright( C) 2000, Intel Corporation, all rights reserved.
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
//(including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort(including negligence or otherwise) arising in any way out of
// the use of this software, even ifadvised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"


/*
   CvEM:
 * params.nclusters    - number of clusters to cluster samples to.
 * means               - calculated by the EM algorithm set of gaussians' means.
 * log_weight_div_det - auxilary vector that k-th component is equal to
                        (-2)*ln(weights_k/det(Sigma_k)^0.5),
                        where <weights_k> is the weight,
                        <Sigma_k> is the covariation matrice of k-th cluster.
 * inv_eigen_values   - set of 1*dims matrices, <inv_eigen_values>[k] contains
                        inversed eigen values of covariation matrice of the k-th cluster.
                        In the case of <cov_mat_type> == COV_MAT_DIAGONAL,
                        inv_eigen_values[k] = Sigma_k^(-1).
 * covs_rotate_mats   - used only if cov_mat_type == COV_MAT_GENERIC, in all the
                        other cases it is NULL. <covs_rotate_mats>[k] is the orthogonal
                        matrice, obtained by the SVD-decomposition of Sigma_k.
   Both <inv_eigen_values> and <covs_rotate_mats> fields are used for representation of
   covariation matrices and simplifying EM calculations.
   For fixed k denote
   u = covs_rotate_mats[k],
   v = inv_eigen_values[k],
   w = v^(-1);
   if <cov_mat_type> == COV_MAT_GENERIC, then Sigma_k = u w u',
   else                                       Sigma_k = w.
   Symbol ' means transposition.
 */

CvEMParams::CvEMParams() : nclusters(10), cov_mat_type(1/*CvEM::COV_MAT_DIAGONAL*/),
    start_step(0/*CvEM::START_AUTO_STEP*/), probs(0), weights(0), means(0), covs(0)
{
    term_crit=cvTermCriteria( CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 100, FLT_EPSILON );
}

CvEMParams::CvEMParams( int _nclusters, int _cov_mat_type, int _start_step,
                        CvTermCriteria _term_crit, const CvMat* _probs,
                        const CvMat* _weights, const CvMat* _means, const CvMat** _covs ) :
                        nclusters(_nclusters), cov_mat_type(_cov_mat_type), start_step(_start_step),
                        probs(_probs), weights(_weights), means(_means), covs(_covs), term_crit(_term_crit)
{}

CvEM::CvEM()
{
    means = weights = probs = inv_eigen_values = log_weight_div_det = 0;
    covs = cov_rotate_mats = 0;
}

CvEM::CvEM( const CvMat* samples, const CvMat* sample_idx,
            CvEMParams params, CvMat* labels )
{
    means = weights = probs = inv_eigen_values = log_weight_div_det = 0;
    covs = cov_rotate_mats = 0;

    // just invoke the train() method
    train(samples, sample_idx, params, labels);
}

CvEM::~CvEM()
{
    clear();
}


void CvEM::clear()
{
    int i;

    cvReleaseMat( &means );
    cvReleaseMat( &weights );
    cvReleaseMat( &probs );
    cvReleaseMat( &inv_eigen_values );
    cvReleaseMat( &log_weight_div_det );

    if( covs || cov_rotate_mats )
    {
        for( i = 0; i < params.nclusters; i++ )
        {
            if( covs )
                cvReleaseMat( &covs[i] );
            if( cov_rotate_mats )
                cvReleaseMat( &cov_rotate_mats[i] );
        }
        cvFree( &covs );
        cvFree( &cov_rotate_mats );
    }
}

void CvEM::read( CvFileStorage* fs, CvFileNode* node )
{
    bool ok = false;
    CV_FUNCNAME( "CvEM::read" );

    __BEGIN__;

    clear();

    size_t data_size;
    CvEMParams _params;
    CvSeqReader reader;
    CvFileNode* em_node = 0;
    CvFileNode* tmp_node = 0;
    CvSeq* seq = 0;
    CvMat **tmp_covs = 0;
    CvMat **tmp_cov_rotate_mats = 0;

    read_params( fs, node );

    em_node = cvGetFileNodeByName( fs, node, "cvEM" );
    if( !em_node )
        CV_ERROR( CV_StsBadArg, "cvEM tag not found" );

    CV_CALL( weights = (CvMat*)cvReadByName( fs, em_node, "weights" ));
    CV_CALL( means = (CvMat*)cvReadByName( fs, em_node, "means" ));
    CV_CALL( log_weight_div_det = (CvMat*)cvReadByName( fs, em_node, "log_weight_div_det" ));
    CV_CALL( inv_eigen_values = (CvMat*)cvReadByName( fs, em_node, "inv_eigen_values" ));

    // Size of all the following data
    data_size = _params.nclusters*2*sizeof(CvMat*);

    CV_CALL( tmp_covs = (CvMat**)cvAlloc( data_size ));
    memset( tmp_covs, 0, data_size );

    tmp_cov_rotate_mats = tmp_covs + params.nclusters;

    CV_CALL( tmp_node = cvGetFileNodeByName( fs, em_node, "covs" ));
    seq = tmp_node->data.seq;
    if( !CV_NODE_IS_SEQ(tmp_node->tag) || seq->total != params.nclusters)
        CV_ERROR( CV_StsParseError, "Missing or invalid sequence of covariance matrices" );
    CV_CALL( cvStartReadSeq( seq, &reader, 0 ));
    for( int i = 0; i < params.nclusters; i++ )
    {
        CV_CALL( tmp_covs[i] = (CvMat*)cvRead( fs, (CvFileNode*)reader.ptr ));
        CV_NEXT_SEQ_ELEM( seq->elem_size, reader );
    }

    CV_CALL( tmp_node = cvGetFileNodeByName( fs, em_node, "cov_rotate_mats" ));
    seq = tmp_node->data.seq;
    if( !CV_NODE_IS_SEQ(tmp_node->tag) || seq->total != params.nclusters)
        CV_ERROR( CV_StsParseError, "Missing or invalid sequence of rotated cov. matrices" );
    CV_CALL( cvStartReadSeq( seq, &reader, 0 ));
    for( int i = 0; i < params.nclusters; i++ )
    {
        CV_CALL( tmp_cov_rotate_mats[i] = (CvMat*)cvRead( fs, (CvFileNode*)reader.ptr ));
        CV_NEXT_SEQ_ELEM( seq->elem_size, reader );
    }

    covs = tmp_covs;
    cov_rotate_mats = tmp_cov_rotate_mats;

    ok = true;
    __END__;

    if (!ok)
        clear();
}

void CvEM::read_params( CvFileStorage *fs, CvFileNode *node)
{
    CV_FUNCNAME( "CvEM::read_params");

    __BEGIN__;

    size_t data_size;
    CvEMParams _params;
    CvSeqReader reader;
    CvFileNode* param_node = 0;
    CvFileNode* tmp_node = 0;
    CvSeq* seq = 0;

    const char * start_step_name = 0;
    const char * cov_mat_type_name = 0;

    param_node = cvGetFileNodeByName( fs, node, "params" );
    if( !param_node )
        CV_ERROR( CV_StsBadArg, "params tag not found" );

    CV_CALL( start_step_name = cvReadStringByName( fs, param_node, "start_step", 0 ) );
    CV_CALL( cov_mat_type_name = cvReadStringByName( fs, param_node, "cov_mat_type", 0 ) );

    if( start_step_name )
        _params.start_step = strcmp( start_step_name, "START_E_STEP" ) == 0 ? START_E_STEP :
                     strcmp( start_step_name, "START_M_STEP" ) == 0 ? START_M_STEP :
                     strcmp( start_step_name, "START_AUTO_STEP" ) == 0 ? START_AUTO_STEP : 0;
    else
        CV_CALL( _params.start_step = cvReadIntByName( fs, param_node, "start_step", -1 ) );


     if( cov_mat_type_name )
        _params.cov_mat_type = strcmp( cov_mat_type_name, "COV_MAT_SPHERICAL" ) == 0 ? COV_MAT_SPHERICAL :
                       strcmp( cov_mat_type_name, "COV_MAT_DIAGONAL" ) == 0 ? COV_MAT_DIAGONAL :
                       strcmp( cov_mat_type_name, "COV_MAT_GENERIC" ) == 0 ? COV_MAT_GENERIC : 0;
    else
        CV_CALL( _params.cov_mat_type = cvReadIntByName( fs, param_node, "cov_mat_type", -1) );

    CV_CALL( _params.nclusters = cvReadIntByName( fs, param_node, "nclusters", -1 ));
    CV_CALL( _params.weights = (CvMat*)cvReadByName( fs, param_node, "weights" ));
    CV_CALL( _params.means = (CvMat*)cvReadByName( fs, param_node, "means" ));

    data_size = _params.nclusters*sizeof(CvMat*);
    CV_CALL( _params.covs = (const CvMat**)cvAlloc( data_size ));
    memset( _params.covs, 0, data_size );
    CV_CALL( tmp_node = cvGetFileNodeByName( fs, param_node, "covs" ));
    seq = tmp_node->data.seq;
    if( !CV_NODE_IS_SEQ(tmp_node->tag) || seq->total != _params.nclusters)
        CV_ERROR( CV_StsParseError, "Missing or invalid sequence of covariance matrices" );
    CV_CALL( cvStartReadSeq( seq, &reader, 0 ));
    for( int i = 0; i < _params.nclusters; i++ )
    {
        CV_CALL( _params.covs[i] = (CvMat*)cvRead( fs, (CvFileNode*)reader.ptr ));
        CV_NEXT_SEQ_ELEM( seq->elem_size, reader );
    }
    params = _params;

    __END__;
}

void CvEM::write_params( CvFileStorage* fs ) const
{
    CV_FUNCNAME( "CvEM::write_params" );

    __BEGIN__;

    const char* cov_mat_type_name =
        (params.cov_mat_type == COV_MAT_SPHERICAL) ? "COV_MAT_SPHERICAL" :
        (params.cov_mat_type == COV_MAT_DIAGONAL) ? "COV_MAT_DIAGONAL" :
        (params.cov_mat_type == COV_MAT_GENERIC) ? "COV_MAT_GENERIC" : 0;

    const char* start_step_name =
        (params.start_step == START_E_STEP) ? "START_E_STEP" :
        (params.start_step == START_M_STEP) ? "START_M_STEP" :
        (params.start_step == START_AUTO_STEP) ? "START_AUTO_STEP" : 0;

    CV_CALL( cvStartWriteStruct( fs, "params", CV_NODE_MAP ) );

    if( cov_mat_type_name )
    {
        CV_CALL( cvWriteString( fs, "cov_mat_type", cov_mat_type_name) );
    }
    else
    {
        CV_CALL( cvWriteInt( fs, "cov_mat_type", params.cov_mat_type ) );
    }

    if( start_step_name )
    {
        CV_CALL( cvWriteString( fs, "start_step", start_step_name) );
    }
    else
    {
        CV_CALL( cvWriteInt( fs, "cov_mat_type", params.start_step ) );
    }

    CV_CALL( cvWriteInt( fs, "nclusters", params.nclusters ));
    CV_CALL( cvWrite( fs, "weights", weights ));
    CV_CALL( cvWrite( fs, "means", means ));

    CV_CALL( cvStartWriteStruct( fs, "covs", CV_NODE_SEQ ));
    for( int i = 0; i < params.nclusters; i++ )
        CV_CALL( cvWrite( fs, NULL, covs[i] ));
    CV_CALL( cvEndWriteStruct( fs ) );

    // Close params struct
    CV_CALL( cvEndWriteStruct( fs ) );

    __END__;
}

void CvEM::write( CvFileStorage* fs, const char* name ) const
{
    CV_FUNCNAME( "CvEM::write" );

    __BEGIN__;

    CV_CALL( cvStartWriteStruct( fs, name, CV_NODE_MAP, CV_TYPE_NAME_ML_EM ) );

    write_params(fs);

    CV_CALL( cvStartWriteStruct( fs, "cvEM", CV_NODE_MAP ) );

    CV_CALL( cvWrite( fs, "means", means ) );
    CV_CALL( cvWrite( fs, "weights", weights ) );
    CV_CALL( cvWrite( fs, "log_weight_div_det", log_weight_div_det ) );
    CV_CALL( cvWrite( fs, "inv_eigen_values", inv_eigen_values ) );

    CV_CALL( cvStartWriteStruct( fs, "covs", CV_NODE_SEQ ));
    for( int i = 0; i < params.nclusters; i++ )
        CV_CALL( cvWrite( fs, NULL, covs[i] ));
    CV_CALL( cvEndWriteStruct( fs ));

    CV_CALL( cvStartWriteStruct( fs, "cov_rotate_mats", CV_NODE_SEQ ));
    for( int i = 0; i < params.nclusters; i++ )
        CV_CALL( cvWrite( fs, NULL, cov_rotate_mats[i] ));
    CV_CALL( cvEndWriteStruct( fs ) );

    // close cvEM
    CV_CALL( cvEndWriteStruct( fs ) );

    // close top level
    CV_CALL( cvEndWriteStruct( fs ) );

    __END__;
}

void CvEM::set_params( const CvEMParams& _params, const CvVectors& train_data )
{
    CV_FUNCNAME( "CvEM::set_params" );

    __BEGIN__;

    int k;

    params = _params;
    params.term_crit = cvCheckTermCriteria( params.term_crit, 1e-6, 10000 );

    if( params.cov_mat_type != COV_MAT_SPHERICAL &&
        params.cov_mat_type != COV_MAT_DIAGONAL &&
        params.cov_mat_type != COV_MAT_GENERIC )
        CV_ERROR( CV_StsBadArg, "Unknown covariation matrix type" );

    switch( params.start_step )
    {
    case START_M_STEP:
        if( !params.probs )
            CV_ERROR( CV_StsNullPtr, "Probabilities must be specified when EM algorithm starts with M-step" );
        break;
    case START_E_STEP:
        if( !params.means )
            CV_ERROR( CV_StsNullPtr, "Mean's must be specified when EM algorithm starts with E-step" );
        break;
    case START_AUTO_STEP:
        break;
    default:
        CV_ERROR( CV_StsBadArg, "Unknown start_step" );
    }

    if( params.nclusters < 1 )
        CV_ERROR( CV_StsOutOfRange, "The number of clusters (mixtures) should be > 0" );

    if( params.probs )
    {
        const CvMat* p = params.probs;
        if( !CV_IS_MAT(p) ||
            (CV_MAT_TYPE(p->type) != CV_32FC1  &&
            CV_MAT_TYPE(p->type) != CV_64FC1) ||
            p->rows != train_data.count ||
            p->cols != params.nclusters )
            CV_ERROR( CV_StsBadArg, "The array of probabilities must be a valid "
            "floating-point matrix (CvMat) of 'nsamples' x 'nclusters' size" );
    }

    if( params.means )
    {
        const CvMat* m = params.means;
        if( !CV_IS_MAT(m) ||
            (CV_MAT_TYPE(m->type) != CV_32FC1  &&
            CV_MAT_TYPE(m->type) != CV_64FC1) ||
            m->rows != params.nclusters ||
            m->cols != train_data.dims )
            CV_ERROR( CV_StsBadArg, "The array of mean's must be a valid "
            "floating-point matrix (CvMat) of 'nsamples' x 'dims' size" );
    }

    if( params.weights )
    {
        const CvMat* w = params.weights;
        if( !CV_IS_MAT(w) ||
            (CV_MAT_TYPE(w->type) != CV_32FC1  &&
            CV_MAT_TYPE(w->type) != CV_64FC1) ||
            (w->rows != 1 && w->cols != 1) ||
            w->rows + w->cols - 1 != params.nclusters )
            CV_ERROR( CV_StsBadArg, "The array of weights must be a valid "
            "1d floating-point vector (CvMat) of 'nclusters' elements" );
    }

    if( params.covs )
        for( k = 0; k < params.nclusters; k++ )
        {
            const CvMat* cov = params.covs[k];
            if( !CV_IS_MAT(cov) ||
                (CV_MAT_TYPE(cov->type) != CV_32FC1  &&
                CV_MAT_TYPE(cov->type) != CV_64FC1) ||
                cov->rows != cov->cols || cov->cols != train_data.dims )
                CV_ERROR( CV_StsBadArg,
                "Each of covariation matrices must be a valid square "
                "floating-point matrix (CvMat) of 'dims' x 'dims'" );
        }

    __END__;
}

/****************************************************************************************/
double CvEM::calcLikelihood( const cv::Mat &input_sample ) const
{
    const CvMat _input_sample = input_sample;
    const CvMat* _sample = &_input_sample ;

    float* sample_data = 0;
    int cov_mat_type = params.cov_mat_type;
    int i, dims = means->cols;
    int nclusters = params.nclusters;

    cvPreparePredictData( _sample, dims, 0, params.nclusters, 0, &sample_data );

    // allocate memory and initializing headers for calculating
    cv::AutoBuffer<double> buffer(nclusters + dims);
    CvMat expo = cvMat(1, nclusters, CV_64F, &buffer[0] );
    CvMat diff = cvMat(1, dims, CV_64F, &buffer[nclusters] );

    // calculate the probabilities
    for( int k = 0; k < nclusters; k++ )
    {
        const double* mean_k = (const double*)(means->data.ptr + means->step*k);
        const double* w = (const double*)(inv_eigen_values->data.ptr + inv_eigen_values->step*k);
        double cur = log_weight_div_det->data.db[k];
        CvMat* u = cov_rotate_mats[k];
        // cov = u w u'  -->  cov^(-1) = u w^(-1) u'
        if( cov_mat_type == COV_MAT_SPHERICAL )
        {
            double w0 = w[0];
            for( i = 0; i < dims; i++ )
            {
                double val = sample_data[i] - mean_k[i];
                cur += val*val*w0;
            }
        }
        else
        {
            for( i = 0; i < dims; i++ )
                diff.data.db[i] = sample_data[i] - mean_k[i];
            if( cov_mat_type == COV_MAT_GENERIC )
                cvGEMM( &diff, u, 1, 0, 0, &diff, CV_GEMM_B_T );
            for( i = 0; i < dims; i++ )
            {
                double val = diff.data.db[i];
                cur += val*val*w[i];
            }
        }
        expo.data.db[k] = cur;
    }

    // probability = (2*pi)^(-dims/2)*exp( -0.5 * cur )
    cvConvertScale( &expo, &expo, -0.5 );
    double factor = -double(dims)/2.0 * log(2.0*CV_PI);
    cvAndS( &expo, cvScalar(factor), &expo );

    // Calculate the log-likelihood of the given sample -
    // see Alex Smola's blog http://blog.smola.org/page/2 for
    // details on the log-sum-exp trick
    double mini,maxi,retval;
    cvMinMaxLoc( &expo, &mini, &maxi, 0, 0 );
    CvMat *flp = cvCloneMat(&expo);
    cvSubS( &expo, cvScalar(maxi), flp);
    cvExp( flp, flp );
    CvScalar ss = cvSum( flp );
    retval = log(ss.val[0]) + maxi;
    cvReleaseMat(&flp);

    if( sample_data != _sample->data.fl )
        cvFree( &sample_data );

    return retval;
}

/****************************************************************************************/
float
CvEM::predict( const CvMat* _sample, CvMat* _probs ) const
{
    float* sample_data   = 0;
    int cls = 0;

    int cov_mat_type = params.cov_mat_type;
    double opt = FLT_MAX;

    int i, dims = means->cols;
    int nclusters = params.nclusters;

    cvPreparePredictData( _sample, dims, 0, params.nclusters, _probs, &sample_data );

    // allocate memory and initializing headers for calculating
    cv::AutoBuffer<double> buffer(nclusters + dims);
    CvMat expo = cvMat(1, nclusters, CV_64F, &buffer[0] );
    CvMat diff = cvMat(1, dims, CV_64F, &buffer[nclusters] );

    // calculate the probabilities
    for( int k = 0; k < nclusters; k++ )
    {
        const double* mean_k = (const double*)(means->data.ptr + means->step*k);
        const double* w = (const double*)(inv_eigen_values->data.ptr + inv_eigen_values->step*k);
        double cur = log_weight_div_det->data.db[k];
        CvMat* u = cov_rotate_mats[k];
        // cov = u w u'  -->  cov^(-1) = u w^(-1) u'
        if( cov_mat_type == COV_MAT_SPHERICAL )
        {
            double w0 = w[0];
            for( i = 0; i < dims; i++ )
            {
                double val = sample_data[i] - mean_k[i];
                cur += val*val*w0;
            }
        }
        else
        {
            for( i = 0; i < dims; i++ )
                diff.data.db[i] = sample_data[i] - mean_k[i];
            if( cov_mat_type == COV_MAT_GENERIC )
                cvGEMM( &diff, u, 1, 0, 0, &diff, CV_GEMM_B_T );
            for( i = 0; i < dims; i++ )
            {
                double val = diff.data.db[i];
                cur += val*val*w[i];
            }
        }

        expo.data.db[k] = cur;
        if( cur < opt )
        {
            cls = k;
            opt = cur;
        }
    }

    // probability = (2*pi)^(-dims/2)*exp( -0.5 * cur )
    cvConvertScale( &expo, &expo, -0.5 );
    double factor = -double(dims)/2.0 * log(2.0*CV_PI);
    cvAndS( &expo, cvScalar(factor), &expo );

    // Calculate the posterior probability of each component
    // given the sample data.
    if( _probs )
    {
        cvExp( &expo, &expo );
        if( _probs->cols == 1 )
            cvReshape( &expo, &expo, 0, nclusters );
        cvConvertScale( &expo, _probs, 1./cvSum( &expo ).val[0] );
    }

    if( sample_data != _sample->data.fl )
        cvFree( &sample_data );

    return (float)cls;
}



bool CvEM::train( const CvMat* _samples, const CvMat* _sample_idx,
                  CvEMParams _params, CvMat* labels )
{
    bool result = false;
    CvVectors train_data;
    CvMat* sample_idx = 0;

    train_data.data.fl = 0;
    train_data.count = 0;

    CV_FUNCNAME("cvEM");

    __BEGIN__;

    int i, nsamples, nclusters, dims;

    clear();

    CV_CALL( cvPrepareTrainData( "cvEM",
        _samples, CV_ROW_SAMPLE, 0, CV_VAR_CATEGORICAL,
        0, _sample_idx, false, (const float***)&train_data.data.fl,
        &train_data.count, &train_data.dims, &train_data.dims,
        0, 0, 0, &sample_idx ));

    CV_CALL( set_params( _params, train_data ));
    nsamples = train_data.count;
    nclusters = params.nclusters;
    dims = train_data.dims;

    if( labels && (!CV_IS_MAT(labels) || CV_MAT_TYPE(labels->type) != CV_32SC1 ||
        (labels->cols != 1 && labels->rows != 1) || labels->cols + labels->rows - 1 != nsamples ))
        CV_ERROR( CV_StsBadArg,
        "labels array (when passed) must be a valid 1d integer vector of <sample_count> elements" );

    if( nsamples <= nclusters )
        CV_ERROR( CV_StsOutOfRange,
        "The number of samples should be greater than the number of clusters" );

    CV_CALL( log_weight_div_det = cvCreateMat( 1, nclusters, CV_64FC1 ));
    CV_CALL( probs  = cvCreateMat( nsamples, nclusters, CV_64FC1 ));
    CV_CALL( means = cvCreateMat( nclusters, dims, CV_64FC1 ));
    CV_CALL( weights = cvCreateMat( 1, nclusters, CV_64FC1 ));
    CV_CALL( inv_eigen_values = cvCreateMat( nclusters,
        params.cov_mat_type == COV_MAT_SPHERICAL ? 1 : dims, CV_64FC1 ));
    CV_CALL( covs = (CvMat**)cvAlloc( nclusters * sizeof(*covs) ));
    CV_CALL( cov_rotate_mats = (CvMat**)cvAlloc( nclusters * sizeof(cov_rotate_mats[0]) ));

    for( i = 0; i < nclusters; i++ )
    {
        CV_CALL( covs[i] = cvCreateMat( dims, dims, CV_64FC1 ));
        CV_CALL( cov_rotate_mats[i]  = cvCreateMat( dims, dims, CV_64FC1 ));
        cvZero( cov_rotate_mats[i] );
    }

    init_em( train_data );
    log_likelihood = run_em( train_data );

    if( log_likelihood <= -DBL_MAX/10000. )
        EXIT;

    if( labels )
    {
        if( nclusters == 1 )
            cvZero( labels );
        else
        {
            CvMat sample = cvMat( 1, dims, CV_32F );
            CvMat prob = cvMat( 1, nclusters, CV_64F );
            int lstep = CV_IS_MAT_CONT(labels->type) ? 1 : labels->step/sizeof(int);

            for( i = 0; i < nsamples; i++ )
            {
                int idx = sample_idx ? sample_idx->data.i[i] : i;
                sample.data.ptr = _samples->data.ptr + _samples->step*idx;
                prob.data.ptr = probs->data.ptr + probs->step*i;

                labels->data.i[i*lstep] = cvRound(predict(&sample, &prob));
            }
        }
    }

    result = true;

    __END__;

    if( sample_idx != _sample_idx )
        cvReleaseMat( &sample_idx );

    cvFree( &train_data.data.ptr );

    return result;
}


void CvEM::init_em( const CvVectors& train_data )
{
    CvMat *w = 0, *u = 0, *tcov = 0;

    CV_FUNCNAME( "CvEM::init_em" );

    __BEGIN__;

    double maxval = 0;
    int i, force_symm_plus = 0;
    int nclusters = params.nclusters, nsamples = train_data.count, dims = train_data.dims;

    if( params.start_step == START_AUTO_STEP || nclusters == 1 || nclusters == nsamples )
        init_auto( train_data );
    else if( params.start_step == START_M_STEP )
    {
        for( i = 0; i < nsamples; i++ )
        {
            CvMat prob;
            cvGetRow( params.probs, &prob, i );
            cvMaxS( &prob, 0., &prob );
            cvMinMaxLoc( &prob, 0, &maxval );
            if( maxval < FLT_EPSILON )
                cvSet( &prob, cvScalar(1./nclusters) );
            else
                cvNormalize( &prob, &prob, 1., 0, CV_L1 );
        }
        EXIT; // do not preprocess covariation matrices,
              // as in this case they are initialized at the first iteration of EM
    }
    else
    {
        CV_ASSERT( params.start_step == START_E_STEP && params.means );
        if( params.weights && params.covs )
        {
            cvConvert( params.means, means );
            cvReshape( weights, weights, 1, params.weights->rows );
            cvConvert( params.weights, weights );
            cvReshape( weights, weights, 1, 1 );
            cvMaxS( weights, 0., weights );
            cvMinMaxLoc( weights, 0, &maxval );
            if( maxval < FLT_EPSILON )
                cvSet( weights, cvScalar(1./nclusters) );
            cvNormalize( weights, weights, 1., 0, CV_L1 );
            for( i = 0; i < nclusters; i++ )
                CV_CALL( cvConvert( params.covs[i], covs[i] ));
            force_symm_plus = 1;
        }
        else
            init_auto( train_data );
    }

    CV_CALL( tcov = cvCreateMat( dims, dims, CV_64FC1 ));
    CV_CALL( w = cvCreateMat( dims, dims, CV_64FC1 ));
    if( params.cov_mat_type != COV_MAT_SPHERICAL )
        CV_CALL( u = cvCreateMat( dims, dims, CV_64FC1 ));

    for( i = 0; i < nclusters; i++ )
    {
        if( force_symm_plus )
        {
            cvTranspose( covs[i], tcov );
            cvAddWeighted( covs[i], 0.5, tcov, 0.5, 0, tcov );
        }
        else
            cvCopy( covs[i], tcov );
        cvSVD( tcov, w, u, 0, CV_SVD_MODIFY_A + CV_SVD_U_T + CV_SVD_V_T );
        if( params.cov_mat_type == COV_MAT_SPHERICAL )
            cvSetIdentity( covs[i], cvScalar(cvTrace(w).val[0]/dims) );
        /*else if( params.cov_mat_type == COV_MAT_DIAGONAL )
            cvCopy( w, covs[i] );*/
        else
        {
            // generic case: covs[i] = (u')'*max(w,0)*u'
            cvGEMM( u, w, 1, 0, 0, tcov, CV_GEMM_A_T );
            cvGEMM( tcov, u, 1, 0, 0, covs[i], 0 );
        }
    }

    __END__;

    cvReleaseMat( &w );
    cvReleaseMat( &u );
    cvReleaseMat( &tcov );
}


void CvEM::init_auto( const CvVectors& train_data )
{
    CvMat* hdr = 0;
    const void** vec = 0;
    CvMat* class_ranges = 0;
    CvMat* labels = 0;

    CV_FUNCNAME( "CvEM::init_auto" );

    __BEGIN__;

    int nclusters = params.nclusters, nsamples = train_data.count, dims = train_data.dims;
    int i, j;

    if( nclusters == nsamples )
    {
        CvMat src = cvMat( 1, dims, CV_32F );
        CvMat dst = cvMat( 1, dims, CV_64F );
        for( i = 0; i < nsamples; i++ )
        {
            src.data.ptr = train_data.data.ptr[i];
            dst.data.ptr = means->data.ptr + means->step*i;
            cvConvert( &src, &dst );
            cvZero( covs[i] );
            cvSetIdentity( cov_rotate_mats[i] );
        }
        cvSetIdentity( probs );
        cvSet( weights, cvScalar(1./nclusters) );
    }
    else
    {
        int max_count = 0;

        CV_CALL( class_ranges = cvCreateMat( 1, nclusters+1, CV_32SC1 ));
        if( nclusters > 1 )
        {
            CV_CALL( labels = cvCreateMat( 1, nsamples, CV_32SC1 ));

            // Not fully executed in case means are already given
            kmeans( train_data, nclusters, labels, cvTermCriteria( CV_TERMCRIT_ITER,
                    params.means ? 1 : 10, 0.5 ), params.means );

            CV_CALL( cvSortSamplesByClasses( (const float**)train_data.data.fl,
                                            labels, class_ranges->data.i ));
        }
        else
        {
            class_ranges->data.i[0] = 0;
            class_ranges->data.i[1] = nsamples;
        }

        for( i = 0; i < nclusters; i++ )
        {
            int left = class_ranges->data.i[i], right = class_ranges->data.i[i+1];
            max_count = MAX( max_count, right - left );
        }
        CV_CALL( hdr = (CvMat*)cvAlloc( max_count*sizeof(hdr[0]) ));
        CV_CALL( vec = (const void**)cvAlloc( max_count*sizeof(vec[0]) ));
        hdr[0] = cvMat( 1, dims, CV_32F );
        for( i = 0; i < max_count; i++ )
        {
            vec[i] = hdr + i;
            hdr[i] = hdr[0];
        }

        for( i = 0; i < nclusters; i++ )
        {
            int left = class_ranges->data.i[i], right = class_ranges->data.i[i+1];
            int cluster_size = right - left;
            CvMat avg;

            if( cluster_size <= 0 )
                continue;

            for( j = left; j < right; j++ )
                hdr[j - left].data.fl = train_data.data.fl[j];

            CV_CALL( cvGetRow( means, &avg, i ));
            CV_CALL( cvCalcCovarMatrix( vec, cluster_size, covs[i],
                &avg, CV_COVAR_NORMAL | CV_COVAR_SCALE ));
            weights->data.db[i] = (double)cluster_size/(double)nsamples;
        }
    }

    __END__;

    cvReleaseMat( &class_ranges );
    cvReleaseMat( &labels );
    cvFree( &hdr );
    cvFree( &vec );
}


void CvEM::kmeans( const CvVectors& train_data, int nclusters, CvMat* labels,
                   CvTermCriteria termcrit, const CvMat* /*centers0*/ )
{
    int i, nsamples = train_data.count, dims = train_data.dims;
    cv::Ptr<CvMat> temp_mat = cvCreateMat(nsamples, dims, CV_32F);
    
    for( i = 0; i < nsamples; i++ )
        memcpy( temp_mat->data.ptr + temp_mat->step*i, train_data.data.fl[i], dims*sizeof(float));
    
    cvKMeans2(temp_mat, nclusters, labels, termcrit, 10);
}


/****************************************************************************************/
/* log_weight_div_det[k] = -2*log(weights_k) + log(det(Sigma_k)))

   covs[k] = cov_rotate_mats[k] * cov_eigen_values[k] * (cov_rotate_mats[k])'
   cov_rotate_mats[k] are orthogonal matrices of eigenvectors and
   cov_eigen_values[k] are diagonal matrices (represented by 1D vectors) of eigen values.

   The <alpha_ik> is the probability of the vector x_i to belong to the k-th cluster:
   <alpha_ik> ~ weights_k * exp{ -0.5[ln(det(Sigma_k)) + (x_i - mu_k)' Sigma_k^(-1) (x_i - mu_k)] }
   We calculate these probabilities here by the equivalent formulae:
   Denote
   S_ik = -0.5(log(det(Sigma_k)) + (x_i - mu_k)' Sigma_k^(-1) (x_i - mu_k)) + log(weights_k),
   M_i = max_k S_ik = S_qi, so that the q-th class is the one where maximum reaches. Then
   alpha_ik = exp{ S_ik - M_i } / ( 1 + sum_j!=q exp{ S_ji - M_i })
*/
double CvEM::run_em( const CvVectors& train_data )
{
    CvMat* centered_sample = 0;
    CvMat* covs_item = 0;
    CvMat* log_det = 0;
    CvMat* log_weights = 0;
    CvMat* cov_eigen_values = 0;
    CvMat* samples = 0;
    CvMat* sum_probs = 0;
    log_likelihood = -DBL_MAX;

    CV_FUNCNAME( "CvEM::run_em" );
    __BEGIN__;

    int nsamples = train_data.count, dims = train_data.dims, nclusters = params.nclusters;
    double min_variation = FLT_EPSILON;
    double min_det_value = MAX( DBL_MIN, pow( min_variation, dims ));
    double _log_likelihood = -DBL_MAX;
    int start_step = params.start_step;
    double sum_max_val;

    int i, j, k, n;
    int is_general = 0, is_diagonal = 0, is_spherical = 0;
    double prev_log_likelihood = -DBL_MAX / 1000., det, d;
    CvMat whdr, iwhdr, diag, *w, *iw;
    double* w_data;
    double* sp_data;

    if( nclusters == 1 )
    {
        double log_weight;
        CV_CALL( cvSet( probs, cvScalar(1.)) );

        if( params.cov_mat_type == COV_MAT_SPHERICAL )
        {
            d = cvTrace(*covs).val[0]/dims;
            d = MAX( d, FLT_EPSILON );
            inv_eigen_values->data.db[0] = 1./d;
            log_weight = pow( d, dims*0.5 );
        }
        else
        {
            w_data = inv_eigen_values->data.db;

            if( params.cov_mat_type == COV_MAT_GENERIC )
                cvSVD( *covs, inv_eigen_values, *cov_rotate_mats, 0, CV_SVD_U_T );
            else
                cvTranspose( cvGetDiag(*covs, &diag), inv_eigen_values );

            cvMaxS( inv_eigen_values, FLT_EPSILON, inv_eigen_values );
            for( j = 0, det = 1.; j < dims; j++ )
                det *= w_data[j];
            log_weight = sqrt(det);
            cvDiv( 0, inv_eigen_values, inv_eigen_values );
        }

        log_weight_div_det->data.db[0] = -2*log(weights->data.db[0]/log_weight);
        log_likelihood = DBL_MAX/1000.;
        EXIT;
    }

    if( params.cov_mat_type == COV_MAT_GENERIC )
        is_general  = 1;
    else if( params.cov_mat_type == COV_MAT_DIAGONAL )
        is_diagonal = 1;
    else if( params.cov_mat_type == COV_MAT_SPHERICAL )
        is_spherical  = 1;
    /* In the case of <cov_mat_type> == COV_MAT_DIAGONAL, the k-th row of cov_eigen_values
    contains the diagonal elements (variations). In the case of
    <cov_mat_type> == COV_MAT_SPHERICAL - the 0-ths elements of the vectors cov_eigen_values[k]
    are to be equal to the mean of the variations over all the dimensions. */

    CV_CALL( log_det = cvCreateMat( 1, nclusters, CV_64FC1 ));
    CV_CALL( log_weights = cvCreateMat( 1, nclusters, CV_64FC1 ));
    CV_CALL( covs_item = cvCreateMat( dims, dims, CV_64FC1 ));
    CV_CALL( centered_sample = cvCreateMat( 1, dims, CV_64FC1 ));
    CV_CALL( cov_eigen_values = cvCreateMat( inv_eigen_values->rows, inv_eigen_values->cols, CV_64FC1 ));
    CV_CALL( samples = cvCreateMat( nsamples, dims, CV_64FC1 ));
    CV_CALL( sum_probs = cvCreateMat( 1, nclusters, CV_64FC1 ));
    sp_data = sum_probs->data.db;

    // copy the training data into double-precision matrix
    for( i = 0; i < nsamples; i++ )
    {
        const float* src = train_data.data.fl[i];
        double* dst = (double*)(samples->data.ptr + samples->step*i);

        for( j = 0; j < dims; j++ )
            dst[j] = src[j];
    }

    if( start_step != START_M_STEP )
    {
        for( k = 0; k < nclusters; k++ )
        {
            if( is_general || is_diagonal )
            {
                w = cvGetRow( cov_eigen_values, &whdr, k );
                if( is_general )
                    cvSVD( covs[k], w, cov_rotate_mats[k], 0, CV_SVD_U_T );
                else
                    cvTranspose( cvGetDiag( covs[k], &diag ), w );
                w_data = w->data.db;
                for( j = 0, det = 0.; j < dims; j++ )
                    det += std::log(w_data[j]);
                if( det < std::log(min_det_value) )
                {
                    if( start_step == START_AUTO_STEP )
                        det = std::log(min_det_value);
                    else
                        EXIT;
                }
                log_det->data.db[k] = det;
            }
            else // spherical
            {
                d = cvTrace(covs[k]).val[0]/(double)dims;
                if( d < min_variation )
                {
                    if( start_step == START_AUTO_STEP )
                        d = min_variation;
                    else
                        EXIT;
                }
                cov_eigen_values->data.db[k] = d;
                log_det->data.db[k] = d;
            }
        }

        if( is_spherical )
        {
            cvLog( log_det, log_det );
            cvScale( log_det, log_det, dims );
        }
    }

    for( n = 0; n < params.term_crit.max_iter; n++ )
    {
        if( n > 0 || start_step != START_M_STEP )
        {
            // e-step: compute probs_ik from means_k, covs_k and weights_k.
            CV_CALL(cvLog( weights, log_weights ));

            sum_max_val = 0.;
            // S_ik = -0.5[log(det(Sigma_k)) + (x_i - mu_k)' Sigma_k^(-1) (x_i - mu_k)] + log(weights_k)
            for( k = 0; k < nclusters; k++ )
            {
                CvMat* u = cov_rotate_mats[k];
                const double* mean = (double*)(means->data.ptr + means->step*k);
                w = cvGetRow( cov_eigen_values, &whdr, k );
                iw = cvGetRow( inv_eigen_values, &iwhdr, k );
                cvDiv( 0, w, iw );

                w_data = (double*)(inv_eigen_values->data.ptr + inv_eigen_values->step*k);

                for( i = 0; i < nsamples; i++ )
                {
                    double *csample = centered_sample->data.db, p = log_det->data.db[k];
                    const double* sample = (double*)(samples->data.ptr + samples->step*i);
                    double* pp = (double*)(probs->data.ptr + probs->step*i);
                    for( j = 0; j < dims; j++ )
                        csample[j] = sample[j] - mean[j];
                    if( is_general )
                        cvGEMM( centered_sample, u, 1, 0, 0, centered_sample, CV_GEMM_B_T );
                    for( j = 0; j < dims; j++ )
                        p += csample[j]*csample[j]*w_data[is_spherical ? 0 : j];
                    //pp[k] = -0.5*p + log_weights->data.db[k];
                    pp[k] = -0.5*(p+CV_LOG2PI * (double)dims) + log_weights->data.db[k];

                    // S_ik <- S_ik - max_j S_ij
                    if( k == nclusters - 1 )
                    {
                        double max_val = pp[0];
                        for( j = 1; j < nclusters; j++ )
                            max_val = MAX( max_val, pp[j] );
                        sum_max_val += max_val;
                        for( j = 0; j < nclusters; j++ )
                            pp[j] -= max_val;
                    }
                }
            }

            CV_CALL(cvExp( probs, probs )); // exp( S_ik )
            cvZero( sum_probs );

            // alpha_ik = exp( S_ik ) / sum_j exp( S_ij ),
            // log_likelihood = sum_i log (sum_j exp(S_ij))
            for( i = 0, _log_likelihood = 0; i < nsamples; i++ )
            {
                double* pp = (double*)(probs->data.ptr + probs->step*i), sum = 0;
                for( j = 0; j < nclusters; j++ )
                    sum += pp[j];
                sum = 1./MAX( sum, DBL_EPSILON );
                for( j = 0; j < nclusters; j++ )
                {
                    double p = pp[j] *= sum;
                    sp_data[j] += p;
                }
                _log_likelihood -= log( sum );
            }
            _log_likelihood+=sum_max_val;

            // Check termination criteria. Use the same termination criteria as it is used in MATLAB
            log_likelihood_delta = _log_likelihood - prev_log_likelihood;
//            if( n>0 )
//                fprintf(stderr, "iter=%d, ll=%0.5f (delta=%0.5f, goal=%0.5f)\n",
//                     n, _log_likelihood, delta,   params.term_crit.epsilon * fabs( _log_likelihood));
             if ( log_likelihood_delta > 0 && log_likelihood_delta < params.term_crit.epsilon * std::fabs( _log_likelihood) )
                 break;
            prev_log_likelihood = _log_likelihood;
        }

        // m-step: update means_k, covs_k and weights_k from probs_ik
        cvGEMM( probs, samples, 1, 0, 0, means, CV_GEMM_A_T );

        for( k = 0; k < nclusters; k++ )
        {
            double sum = sp_data[k], inv_sum = 1./sum;
            CvMat* cov = covs[k], _mean, _sample;

            w = cvGetRow( cov_eigen_values, &whdr, k );
            w_data = w->data.db;
            cvGetRow( means, &_mean, k );
            cvGetRow( samples, &_sample, k );

            // update weights_k
            weights->data.db[k] = sum;

            // update means_k
            cvScale( &_mean, &_mean, inv_sum );

            // compute covs_k
            cvZero( cov );
            cvZero( w );

            for( i = 0; i < nsamples; i++ )
            {
                double p = probs->data.db[i*nclusters + k]*inv_sum;
                _sample.data.db = (double*)(samples->data.ptr + samples->step*i);

                if( is_general )
                {
                    cvMulTransposed( &_sample, covs_item, 1, &_mean );
                    cvScaleAdd( covs_item, cvRealScalar(p), cov, cov );
                }
                else
                    for( j = 0; j < dims; j++ )
                    {
                        double val = _sample.data.db[j] - _mean.data.db[j];
                        w_data[is_spherical ? 0 : j] += p*val*val;
                    }
            }

            if( is_spherical )
            {
                d = w_data[0]/(double)dims;
                d = MAX( d, min_variation );
                w->data.db[0] = d;
                log_det->data.db[k] = d;
            }
            else
            {
                // Det. of general NxN cov. matrix is the prod. of the eig. vals
                if( is_general )
                    cvSVD( cov, w, cov_rotate_mats[k], 0, CV_SVD_U_T );
                cvMaxS( w, min_variation, w );
                for( j = 0, det = 0.; j < dims; j++ )
                    det += std::log( w_data[j] );
                log_det->data.db[k] = det;
            }
        }

        cvConvertScale( weights, weights, 1./(double)nsamples, 0 );
        cvMaxS( weights, DBL_MIN, weights );

        if( is_spherical )
        {
            cvLog( log_det, log_det );
            cvScale( log_det, log_det, dims );
        }
    } // end of iteration process

    //log_weight_div_det[k] = -2*log(weights_k/det(Sigma_k))^0.5) = -2*log(weights_k) + log(det(Sigma_k)))
    if( log_weight_div_det )
    {
        cvScale( log_weights, log_weight_div_det, -2 );
        cvAdd( log_weight_div_det, log_det, log_weight_div_det );
    }

    /* Now finalize all the covariation matrices:
    1) if <cov_mat_type> == COV_MAT_DIAGONAL we used array of <w> as diagonals.
       Now w[k] should be copied back to the diagonals of covs[k];
    2) if <cov_mat_type> == COV_MAT_SPHERICAL we used the 0-th element of w[k]
       as an average variation in each cluster. The value of the 0-th element of w[k]
       should be copied to the all of the diagonal elements of covs[k]. */
    if( is_spherical )
    {
        for( k = 0; k < nclusters; k++ )
            cvSetIdentity( covs[k], cvScalar(cov_eigen_values->data.db[k]));
    }
    else if( is_diagonal )
    {
        for( k = 0; k < nclusters; k++ )
            cvTranspose( cvGetRow( cov_eigen_values, &whdr, k ),
                         cvGetDiag( covs[k], &diag ));
    }
    cvDiv( 0, cov_eigen_values, inv_eigen_values );

    log_likelihood = _log_likelihood;

    __END__;

    cvReleaseMat( &log_det );
    cvReleaseMat( &log_weights );
    cvReleaseMat( &covs_item );
    cvReleaseMat( &centered_sample );
    cvReleaseMat( &cov_eigen_values );
    cvReleaseMat( &samples );
    cvReleaseMat( &sum_probs );

    return log_likelihood;
}


int CvEM::get_nclusters() const
{
    return params.nclusters;
}

const CvMat* CvEM::get_means() const
{
    return means;
}

const CvMat** CvEM::get_covs() const
{
    return (const CvMat**)covs;
}

const CvMat* CvEM::get_weights() const
{
    return weights;
}

const CvMat* CvEM::get_probs() const
{
    return probs;
}

using namespace cv;

CvEM::CvEM( const Mat& samples, const Mat& sample_idx, CvEMParams params )
{
    means = weights = probs = inv_eigen_values = log_weight_div_det = 0;
    covs = cov_rotate_mats = 0;
    
    // just invoke the train() method
    train(samples, sample_idx, params);
}    

bool CvEM::train( const Mat& _samples, const Mat& _sample_idx,
                 CvEMParams _params, Mat* _labels )
{
    CvMat samples = _samples, sidx = _sample_idx, labels, *plabels = 0;
    
    if( _labels )
    {
        int nsamples = sidx.data.ptr ? sidx.rows : samples.rows;
        
        if( !(_labels->data && _labels->type() == CV_32SC1 &&
              (_labels->cols == 1 || _labels->rows == 1) &&
              _labels->cols + _labels->rows - 1 == nsamples) )
            _labels->create(nsamples, 1, CV_32SC1);
        plabels = &(labels = *_labels);
    }
    return train(&samples, sidx.data.ptr ? &sidx : 0, _params, plabels);
}

float
CvEM::predict( const Mat& _sample, Mat* _probs ) const
{
    CvMat sample = _sample, probs, *pprobs = 0;
    
    if( _probs )
    {
        int nclusters = params.nclusters;
        if(!(_probs->data && (_probs->type() == CV_32F || _probs->type()==CV_64F) &&
             (_probs->cols == 1 || _probs->rows == 1) &&
             _probs->cols + _probs->rows - 1 == nclusters))
            _probs->create(nclusters, 1, _sample.type());
        pprobs = &(probs = *_probs);
    }
    return predict(&sample, pprobs);
}

int CvEM::getNClusters() const
{
    return params.nclusters;
}

Mat CvEM::getMeans() const
{
    return Mat(means);
}

void CvEM::getCovs(vector<Mat>& _covs) const
{
    int i, n = params.nclusters;
    _covs.resize(n);
    for( i = 0; i < n; i++ )
        Mat(covs[i]).copyTo(_covs[i]);
}

Mat CvEM::getWeights() const
{
    return Mat(weights);
}

Mat CvEM::getProbs() const
{
    return Mat(probs);
}


/* End of file. */
