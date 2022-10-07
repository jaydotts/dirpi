
// Various helper functions 

void printV( std::vector<double> const &a){
  for (int i{0};i<a.size();i++){
    cout<< a.at(i)<< ' ';
  }
}

double CalcScalar(int nSLWCLK, int channel) {
	auto t0_ch = t0[channel];
    auto t1 = std::chrono::high_resolution_clock::now();
    double nTrigs = double(pow(2,14) - nSLWCLK); 
    //double tDiff = (t1.time_since_epoch().count()-t0_ch.time_since_epoch().count());
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0_ch);
    double tDiff = duration.count();
    cout<<tDiff<<" microseconds"<<std::endl;
	return 1000000*nTrigs/tDiff;
}

int ReadScalars() {
	//t0 = std::chrono::high_resolution_clock::now();
	if (checkScalars[0] == 0 && checkScalars[1] == 0) return 1; // return 1 if both were checked
	
	nSLWCLK += 1;
	
	// last read bits become "previous" bits
	trgCnt[0][1] = trgCnt[0][0];
	trgCnt[1][1] = trgCnt[1][0];
	//read new bits
	trgCnt[0][0] = ReadPin(Trg1Cnt);
	trgCnt[1][0] = ReadPin(Trg2Cnt);
	
	for (int ch = 0; ch < 2; ch++) {
		if (checkScalars[ch]) {
			if (trgCnt[ch][1] == 1 && trgCnt[ch][0] == 0) {
				scalars[ch] = CalcScalar(nSLWCLK, ch);
				cout << "DEBUG ==== " << nSLWCLK << std::endl;
				cout << "DEBUG ==== " << scalars[ch] << " Hz" << std::endl;
				checkScalars[ch] = 0;
				t0[ch] = std::chrono::high_resolution_clock::now();
			}
		}
	}
	return 0;
}

std::vector<double> GetArea(int dataBlock[][4096], int ch, int thr, int minWidth =1, int nQuiet = 5){
  std::vector<double> sums; 
  double sum{0};
  double interval = 31.25; // time between samples in ns
  bool isQuiet=true, isTrig=false, isDone =false;
  int width = 0;
  int imax = 0;
  double max = 0;
  //loop over waveform entries
  for(int i= nQuiet+1;i<4096; i++){
    if (isTrig ==false){
      width =0.; sum=0.; max = 0; imax =0; isQuiet=true;
      for ( int j =0; j<nQuiet;j++){
        if (dataBlock[ch][i-j-1]>thr){isQuiet=false;} 
      }
    }
    if (isQuiet && dataBlock[ch][i]>thr){
      isTrig = true;
      sum+=double(dataBlock[ch][i])*interval;
      width+=1;
 //     sums.push_back(sum);     
     // cout <<i << ' ' << width<<' ';
      if (dataBlock[ch][i]>max){ max = dataBlock[ch][i]; imax = i;}
    }else{
      isTrig=false;
      //cout<<sum<<endl;
      if (width>minWidth){ sums.push_back(sum);   sums.push_back(imax); //cout<<"ch: "<<ch<<" "<<"width "<<width <<endl; 
      }
    }
  }  
 // printV(sums); cout<<endl;
  return sums;
}

