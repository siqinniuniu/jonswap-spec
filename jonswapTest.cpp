//
//  jonswapTest.cpp
//  
//
//  Created by Munan Xu on 7/2/15.
//
//
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include "jonswapSpec.h"

using std::ostream;
using std::ofstream;
using std::cout;
using std::endl;

int main() {
    
    vector<double> w;
    vector<double> dist;
    vector<double> wc, amps;
    set<double> bounds;
    vector<double>::iterator wc_it, amps_it;
    set<double>::iterator bounds_it;
    vector<double>::iterator it;
    vector<double>::iterator dist_it;
    
    jonswapSpec jonswap = jonswapSpec(.05);
    
    jonswap.bin(10);
    amps = jonswap.calcPaddleAmps(0.01, 0.75);
    bounds = jonswap.getBins();
    wc = jonswap.getWCs();
    
    cout << jonswap;
    cout << "\n------------------------------------\n" << endl;
    
    cout << "Bin\t\t\tW_c\t\tAmp" << endl;
    
    amps_it = amps.begin();
    wc_it = wc.begin();
    
    for (bounds_it = bounds.begin(); bounds_it != bounds.end(); ++bounds_it) {
        if (bounds_it == bounds.begin()) {
            cout << "0 - " << *bounds_it << "\t\t: " << *wc_it << "\t" << *amps_it << endl;
        } else {
            cout << *prev(bounds_it) << " - " << *bounds_it << "\t: " << *wc_it << "\t" << *amps_it << endl;
        }
        
        wc_it++;
        amps_it++;
    }
    cout << *prev(bounds.end()) << " - " << jonswap.getWmax() << "\t\t: " << *wc_it << "\t" << *amps_it << endl;
    
    for (double i = 0.0; i < 3; i += 0.001 )
        w.push_back(i);
    
    dist = jonswap.getamp(w);
    
    dist_it = dist.begin();
    
    ofstream data;
    data.open("jonswap_spec.txt",std::ofstream::out|std::ofstream::trunc);
    
    data << "w\t\tamp"<< endl;
    for (it = w.begin(); it != w.end(); ++it) {
        data << *it << "\t" << *dist_it <<endl;
        dist_it++;
    }
    data.close();
    
}