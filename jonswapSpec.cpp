//
//  jonswapSpec.cpp
//  
//
//  Created by Munan Xu on 6/30/15.
//
//   S_j (omega) = alpha g^2/omega^5 * exp[-1.2 * (omega_p/omega)^4] * gamma^r
//
//  where r is defined as:
//
//  r = exp[ -(omega - omega_p)^2 / (2*sigma^2*omega_p^2) ]
//
//  Need U_10 (10 m wind speed) and F (fetch) to determine
//     alpha = 0.076 (U_10^2/(F g))^.22
//   omega_p = 22 (g*g/(U_10*F)^(1/3), peak frequency
//     gamma = 3.3
//     sigma = (0.07 for omega<omega_p; 0.09 for omega>omega_p)
//

#include <stdio.h>
#include <math.h>
#include <float.h>
#include "jonswapSpec.h"


// Default constructor uses pre-defined jonswap parameters
// alpha - ratio of windspeed to fetch
// wp - dominant period
// wmax - upper bound of jonswap spectrum
// gamma - peak sharpening factor
// s1 - variance when w =< wp
// s2 - variance when w > wp
jonswapSpec::jonswapSpec(double alpha, double wp, double wmax, double gamma, double s1, double s2) {
	this->alpha = alpha;
	this->wp = wp;
	this->wmax = wmax;
	this->gamma = gamma;
	this->s1 = s1;
	this->s2 = s2;
	
	this->F = 0.0;
	this->vel10 = 0.0;
	
	g = 9.81;
}

// Initializer that calculates alpha and wp based on:
// vel10 : wind speed at 10m (m/s)
// F : fetch (m) (length of ocean constant wind speed)
jonswapSpec::jonswapSpec(double vel10, double F) {
	
	this->vel10 = vel10;
	this->F = F;
	g = 9.81;
	
	alpha = calcAlpha();
	wp = calcWp();
	
	wmax = 33*wp / (2*M_PI);
	
	gamma = 3.3;
	s1 = 0.7;
	s2 = 0.9;
}

jonswapSpec::~jonswapSpec() {
    
}


// Calculate amplitude of jonswap spectrum for specific angular velocity
double jonswapSpec::getamp(double w) {
	
	// Compute intermediate values:
	double s = (w > wp ? s2 : s1); // set sigma 
	double dw = w - wp;
	double rexp_denom = s * wp ;
	double rexp = -pow(dw/rexp_denom, 2) / 2.0;
	double r = exp(rexp);
	
	double gammaR = pow(gamma,r);
	
	double jdistExp = -1.2 * pow(wp/w,4);
	double c = alpha * g * g * pow(w, -5); // alpha g^2/w^5
	
	return c * exp(jdistExp) * gammaR;
	
}

// Calculate amplitudes of jonswap spectrum for a vector of angular velocities
vector<double> jonswapSpec::getamp(vector<double> w) {
	vector<double> amps;
	vector<double>::iterator it;
	double amp;
	
	for ( it = w.begin()+1; it != w.end(); ++it) {      //RAD: +1
		amp = getamp(*it);
		amps.push_back(amp);
	}
	return amps;
}

// Randomly generate boundaries for N bins and calculate their center frequency
void jonswapSpec::bin(int n) {
	
#if USE_CPP11
	random_device gen;
	normal_distribution<double> distribution(wp, wp/2);
	//auto bound = std::bind(distribution, gen);
	cout << "Bounds (Normal Dist): " <<endl;
	cout << "mu " << wp << ", sigma " << wp/2 << endl;
	double bound;
	while (bounds.size() < n-1) {
		bound = distribution(gen);
		if(bound > 0 && bound < wmax) {
			bounds.insert(bound);
		}
		
	}
	for(auto b_it = bounds.begin(); b_it != bounds.end(); ++b_it) {
		cout << *b_it << endl;
	}
#else
	srand(time(NULL));
	double bound;
	double range = 0.4 * (wmax / n); // range is +- 2.5% of bin width 
	double offset = -wmax / (2*n); // center range around 0
	
	cout<<"Bounds (Uniform Generator): " <<endl;
	
	for (int i = 1; i < n; i++) {
		bound = i * wmax/n + ((double) rand() / RAND_MAX) * range + offset;
		cout <<bound <<endl;
		bounds.insert(bound);
	}
#endif

	set<double>::iterator it;
	

	/*it = bounds.begin();
	cout << "bounds: 0 - " << *next(it);
	wc.push_back(*next(it)/2);
	cout << ":\twc = " << *next(it)/2 << endl;*/

	for (it = bounds.begin(); it != prev(bounds.end()); ++it) {
		double currBound = *it;
		double nextBound = *next(it);

		//cout << "currBound: "<< currBound << "\tnextBound: " << nextBound << endl;

		if (it == bounds.begin()) {
			cout << "bounds: 0 - " << currBound;
			wc.push_back(currBound/2);
			cout <<":\twc = " << currBound/2 << endl;
		}
		cout << "bounds: " << currBound << " - " << nextBound;
		wc.push_back((currBound + nextBound)/2);
		cout << ":\twc = " << (currBound + nextBound)/2 << endl;
	}
	
	cout << "bounds: " << *prev(bounds.end()) << " - " << wmax;
	wc.push_back((*prev(bounds.end()) + wmax)/2);
	cout << ":\twc = " << (*prev(bounds.end()) + wmax)/2 << endl;
}

// Calculate alpha based on wind speed and fetch
// Helper function for automatic parameter calculation constructor
double jonswapSpec::calcAlpha() {
	double velDivF = vel10/F;

	double tmp = velDivF * vel10 / g;

	return 0.076 * pow(tmp, 0.22);
}

// Calculate peak angular velocity of jonswap spectrum from wind speed and fetch
// Helper function for automatic parameter calculation constructor
double jonswapSpec::calcWp() {
	double velxF = vel10*F;
	double g2 = g*g;

	return 22 * cbrt(g2/velxF);
}


// Integrate jonswap spectrum using discrete trapezoidal integration to find amp of bin
vector <double> jonswapSpec::calcBinAmps (int nmems)   {
	set<double>::iterator bounds_it = bounds.begin();
	double binWidth;
	double binArea, area;

	double startx, dx;
	double sidea, sideb, slice_area;

	area = 0;
	binArea = 0;

	binWidth = *bounds_it;
	dx = binWidth/(nmems+1); // start at dx instead of 0 so we need an extra step
	startx = dx;

	cout << "bounds: " << startx << " - " << startx + binWidth;

	for(double i = startx; i < startx+binWidth; i += dx) {
		sidea = this->getamp(i);
		sideb = this->getamp(i + dx);

		slice_area = dx*(sidea + sideb)/2;
		binArea += slice_area;
	}

	amps.push_back(binArea);
	area += binArea;
	cout << "\tbinArea = " << binArea << endl;

	for (bounds_it = bounds.begin(); bounds_it != bounds.end(); ++bounds_it) {
		binArea = 0;

		if (bounds_it == prev(bounds.end())) {
			startx = *bounds_it;
			binWidth = wmax - startx;
			dx = binWidth/nmems;
		} else {
			startx = *bounds_it;
			binWidth = *next(bounds_it) - startx;
			dx = binWidth/nmems;
		}

		cout << "bounds: " << startx << " - " << startx + binWidth;

		for(double i = startx; i < startx+binWidth; i += dx) {
			sidea = this->getamp(i);
			sideb = this->getamp(i + dx);

			slice_area = dx*(sidea + sideb)/2;
			binArea += slice_area;
		}
		amps.push_back(binArea/binWidth);
		area += binArea/binWidth;
		cout << "\tbinArea = " << binArea/binWidth << endl;
	}
	cout << "finished calculating areas... total area is: " << area << endl;
	return amps;
}



// calculate actual paddle stokes as a function of center frequency using linear wave theory
vector<double> jonswapSpec::calcPaddleAmps(double h) {

	vector<double>::iterator iamps = amps.begin();
	vector<double>::iterator wc_it;
	set<double>::iterator bounds_it = bounds.begin();
	bool piston = false;
	double HoS;
	double binWidth;

	for (wc_it = wc.begin(); wc_it !=wc.end(); ++wc_it) {
		if (wc_it == prev(wc.end())) {
			binWidth = wmax - *bounds_it;
			cout << "wc " << *wc_it << ", lb " << *bounds_it << ", ub " << wmax << ", iamp " << *iamps;
		} else if (wc_it == wc.begin()) {
			binWidth = *bounds_it;
			cout << "wc " << *wc_it << ", lb 0, ub " << *bounds_it << ", iamp " << *iamps;
		} else {
			binWidth = (*next(bounds_it) -*bounds_it);
			cout << "wc " << *wc_it << ", lb " << *bounds_it << ", ub " << *next(bounds_it) << ", iamp " << *iamps;
			bounds_it++;
		}

		double k0 = (*wc_it)*(*wc_it)/9.81;	  //k0= wc^2/g
		double kh = k0*h*pow(1.0-exp(-(pow(k0*h, 1.25))),-.4);
		//cout <<"wc  "<< *wc_it << ", k0 " << k0 <<", kh " <<kh;

		double tempAmp=sqrt(*iamps*binWidth*2);
		if ( piston ) {
			HoS= 2*(cosh(2*kh)-1)/(sinh(2*kh) + 2*kh);
			cout<< ", Piston Wavemaker" <<endl;
		} else {
			HoS= 4*(sinh(kh)/kh)*(kh*sinh(kh)-cosh(kh)+1)/(sinh(2*kh)+2*kh);
			cout<< ", Flap Wavemaker" ;
		}
		tempAmp = tempAmp/HoS;
		cout <<", PaddleAmp " << tempAmp <<endl;
		paddleAmps.push_back(tempAmp/HoS);

 	   //cout <<*iamps<<", " << binWidth<<", "<< sqrt(*iamps*binWidth*2)<<endl;
		iamps++;
	}
	return paddleAmps;
}


ostream &operator<<(ostream &output, const jonswapSpec & jonswap) {
	output << "jonswapSpec params:" << endl
		<< "alpha\t: " << jonswap.alpha << endl
		<< "gamma\t: " << jonswap.gamma << endl
		<< "w_p\t: " << jonswap.wp << endl
		<< "w_max\t: " << jonswap.wmax << endl
		<< "s1\t: " << jonswap.s1 << " | (w <= w_p)" << endl
		<< "s2\t: " << jonswap.s2 << " | (w > w_p)" << endl;
	if (jonswap.vel10 > 0 && jonswap.F > 0) {
		output << "vel10\t: " << jonswap.vel10 << endl
		<< "F\t: " << jonswap.F << endl;
	}
	if (jonswap.amps.size()) {
		output << "Nbins\t: " << jonswap.bounds.size() + 1 << endl;
		output << "Amps\t: [ 1 x " << jonswap.amps.size() << " ]\n";
		output << "W_c\t: [ 1 x " << jonswap.wc.size() << " ]\n";
	}

	return output;
}



