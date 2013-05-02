/* 
 * Copyright 2002, Statistics Research, Bell Labs, Lucent Technologies and
 * The University of North Carolina at Chapel Hill
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Reference
 *     Stochastic Models for Generating Synthetic HTTP Source Traffic 
 *     J. Cao, W.S. Cleveland, Y. Gao, K. Jeffay, F.D. Smith, and M.C. Weigle 
 *     IEEE INFOCOM 2004.
 *
 * Documentation available at http://dirt.cs.unc.edu/packmime/
 * 
 * Contacts: Michele Weigle (mcweigle@cs.unc.edu),
 *           Kevin Jeffay (jeffay@cs.unc.edu)
 */

#include "ranvar.h"

#define PACKMIME_XMIT_CLIENT 0
#define PACKMIME_XMIT_SERVER 1
#define PACKMIME_REQ_SIZE 0
#define PACKMIME_RSP_SIZE 1

struct arima_params {
  double d;
  int N;
  double varRatioParam0, varRatioParam1;
  int pAR, qMA;
};

/*:::::::::::::::::::::::::::::::: FX  ::::::::::::::::::::::::::::::::::::::*/

// FX generator based on interpolation
class FX {
public:
  FX(const double* x, const double* y, int n);
  ~FX();
  double LinearInterpolate(double xnew);
private:
  double* x_, *y_;
  int nsteps_;
  double* slope_; // avoid real-time slope computation
};

/*:::::::::::::::::::::::::::: Fractional ARIMA  ::::::::::::::::::::::::::::*/

class FARIMA {
public:
  FARIMA(RNG* rng, double d, int N, int pAR=0, int qMA=1);
  ~FARIMA();

  double Next();
private:
  RNG* rng_;
  int t_, N_, pAR_, qMA_, tmod_;
  double* AR_, *MA_, *x_, *y_, *phi_, d_;
  double NextLow();

};



/*:::::::::::::::::::::: PackMimeHTTP Transmission Delay RanVar :::::::::::::::::*/

class PackMimeHTTPXmitRandomVariable : public RandomVariable {

public:
  virtual double value();
  virtual double avg();
  PackMimeHTTPXmitRandomVariable();
  PackMimeHTTPXmitRandomVariable(double rate, int type);
  PackMimeHTTPXmitRandomVariable(double rate, int type, RNG* rng);
  ~PackMimeHTTPXmitRandomVariable();
  void initialize();
  double* ratep() {return &rate_;}
  double rate() {return rate_;}
  void setrate(double r) {rate_ = r;}
  int* typep() {return &type_;}
  int type() {return type_;}
  void settype(int t) {type_ = t;}

private:
  double rate_;
  int type_;
  double varRatio_, sigmaNoise_, sigmaEpsilon_;
  double const_;
  double mean_;
  FARIMA* fARIMA_;
  RNG* myrng_;
  static const double short_rtt_invcdf_x[];
  static const double short_rtt_invcdf_y[];
  static const double long_rtt_invcdf_x[];
  static const double long_rtt_invcdf_y[];
  static struct arima_params rtt_arima_params[]; 
  static FX rtt_invcdf[2];
  static const double weibull_shape[2];
};

/*:::::::::::::::::::::: PackMimeHTTP Flow Arrival RanVar :::::::::::::::::::::::*/

class PackMimeHTTPFlowArriveRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg();
  virtual double avg(int gap_type_);
  double* value_array();
  PackMimeHTTPFlowArriveRandomVariable();
  PackMimeHTTPFlowArriveRandomVariable(double rate);
  PackMimeHTTPFlowArriveRandomVariable(double rate, RNG* rng);
  ~PackMimeHTTPFlowArriveRandomVariable();
  void initialize();
  double* ratep() {return &rate_;}
  double rate() {return rate_;}
  void setrate(double r) {rate_ = r;}
  void setnpage(double npage_);
  void setntrans(double ntrans_);

private:
  int Template (int pages, int *objs_per_page);
  double rate_;
  double varRatio_, sigmaNoise_, sigmaEpsilon_, weibullShape_, weibullScale_;
  FARIMA *fARIMA_;
  RNG* myrng_;
  static struct arima_params flowarrive_arima_params;
  static const double shapeParam0 = -1.96311;
  static const double shapeParam1 =  1.275;
  static const double shapeParam2 =  0.389;

  static const double p_persistent = 0.09;  /* P(persistent=1) */

  /* for generating num of page reqs and num of transfers for each req */
  static const double p_1page=0.82; /* P(#page reqs=1) */
  static const double shape_npage = 1;  /* shape param for (#page reqs-1)*/
  //  double scale_npage; /* scale param for (#page reqs-1) 0.417 */
  static const double scale_npage = 0.417;
  static const double shape_ntransfer = 1; /* shape param for (#xfers-1)*/
  //  double scale_ntransfer; /* scale param for (#xfers-1) 1.578 */
  static const double scale_ntransfer = 1.578;
  static const double p_1transfer = 0.69; /* P(#xfers=1 | #page req>=2) */

  /* time gap within page requests */
  static const double m_loc_w = -4.15; /* mean of location */
  static const double v_loc_w = 3.12;  /* variance of location */
  static const double shape_scale2_w = 2.35; /* Gamma shape param of scale^2*/
  static const double rate_scale2_w = 2.35; /* Gamma rate param of scale^2 */
  static const double v_error_w = 1.57; /* variance of error */

  /* time gap between page requests */
  static const double m_loc_b = 3.22; /* mean of location */
  static const double v_loc_b = 0.73; /* variance of location */
  static const double shape_scale2_b = 1.85; /* Gamma shape param of scale^2*/
  static const double rate_scale2_b = 1.85; /* Gamma rate param of scale^2 */
  static const double v_error_b = 1.21; /* variance of error */
};

/*:::::::::::::::::::::: PackMimeHTTP File Size RanVar :::::::::::::::::::::::::*/

class PackMimeHTTPFileSizeRandomVariable : public RandomVariable {
public:
  virtual double value();
  virtual double avg();
  double* value_array(int);
  PackMimeHTTPFileSizeRandomVariable();
  PackMimeHTTPFileSizeRandomVariable(double rate, int type);  
  PackMimeHTTPFileSizeRandomVariable(double rate, int type, RNG* rng);  
  ~PackMimeHTTPFileSizeRandomVariable(); 
  void initialize();
  double* ratep() {return &rate_;}
  double rate() {return rate_;}
  void setrate(double r) {rate_ = r;}
  int* typep() {return &type_;}
  int type() {return type_;}
  void settype(int t) {type_ = t;}

private:
  double rate_;
  int type_;
  double varRatio_, sigmaNoise_, sigmaEpsilon_;
  FARIMA* fARIMA_;
  RNG* myrng_;
  int runlen_, state_;
  int yold_;
  double shape_[2], scale_[2];
  int rfsize0(int state);
  int qfsize1(double p);

  /* fitted inverse cdf curves for file sizes  */
  static const double fsize0_invcdf_a_x[];
  static const double fsize0_invcdf_a_y[];
  static const double fsize0_invcdf_b_x[];
  static const double fsize0_invcdf_b_y[];
  static const double fsize1_invcdf_a_x[];
  static const double fsize1_invcdf_a_y[];
  static const double fsize1_invcdf_b_x[];
  static const double fsize1_invcdf_b_y[];
  static const double fsize1_prob_a=0.5;
  static const double fsize1_d=0.31; 
  static const double fsize1_varRatio_intercept=0.123;
  static const double fsize1_varRatio_slope=0.494;

  /* server file size run length for downloaded and cache validation */
  static const double weibullScaleCacheRun = 1.1341;
  static const double weibullShapeCacheRun_Asymptoe = 0.82;
  static const double weibullShapeCacheRun_Para1 = 0.6496;
  static const double weibullShapeCacheRun_Para2 = 0.0150;
  static const double weibullShapeCacheRun_Para3 = 1.5837;
  static const double weibullScaleDownloadRun = 3.0059;
  static const double weibullShapeDownloadRun = 0.82;
  static const double fsize0_para[];
  static struct arima_params filesize_arima_params;
  static const double* p;
  static FX fsize_invcdf[2][2]; 

  /* cut off of file size for cache validation */
  static const int fsize0_cache_cutoff=275; 
  static const int fsize0_stretch_thres=400;
  /* mean of log2(fsize0) for non cache validation flows */
  static const double m_fsize0_notcache = 10.50;
  /* variance of log2(fsize0) for non cache validation flows */ 
  static const double v_fsize0_notcache = 3.23; 

  /* fsize0 for non-cache validation flow */
  static const double m_loc = 10.62; /* mean of location */
  static const double v_loc = 0.94; /* variance of location */
  static const double shape_scale2 = 3.22;/*Gamma shape parameter of scale^2*/
  static const double rate_scale2 = 3.22; /*Gamma rate parameter of scale^2 */
  static const double v_error = 2.43; /* variance of error */
};

