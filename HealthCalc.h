/*
 * Classes, a namespace and methods for calculating health measures (QALY, FI at various times)
 */

#ifndef HEALTHCALC_H_
#define HEALTHCALC_H_

// Object to store the FI curve of each individual
// To reduce the amount of data stored and make averaging easier,
// the FI curves just have FI values at regularly spaced ages (usually 0, 1, 2, ...)
class FICurve {
public:

	// Shortcoming: you need to anticipate the max length of FIcurve you will see
	// If you don't play around with the GNM parameters, an FIcurvelength of 150 will be plenty
	// (which is what I've reserved just to be safe)
	FICurve();

	std::vector<float> getFIcurvetot() const {return FIcurvetot;}
	std::vector<float> getFIcurve30() const {return FIcurve30;}


	void fillRemianingFIcurve(){

		for (int i = n; i < FIcurvelength; i++){
			FIcurvetot.push_back(-1.0);
			FIcurve30.push_back(-1.0);
		}
	}

	void createSampleTimes(float timeStep){

		float time = 0.0;
		for (int i = 0; i < FIcurvelength; i++){
			sampleTimes.push_back(time);
			time += timeStep;
		}
	}

	float getSampleTime() const {return (sampleTimes[n]);}

	void sampleFI(int N, int deficits_tot, int deficits30){

		float FItot = static_cast<float>(deficits_tot)/N; // forcing non integer division
		float FI30 = static_cast<float>(deficits30)/30.0;

		//std::cout << "FI sampled for time " << sampleTimes[n] << std::endl;
		//std::cout << "FItot: " << FItot << ", FI30: " << FI30 << std::endl;

		FIcurvetot.push_back(FItot);
		FIcurve30.push_back(FI30);
		n++;
	}

	void resetFIcurves(){
		FIcurvetot.clear();
		FIcurve30.clear();
		FIcurvetot.push_back(0.0);
		FIcurve30.push_back(0.0);
		n = 1;
	}


private:
	std::vector<float> FIcurvetot; // FItot -> fraction of all nodes damaged
	std::vector<float> FIcurve30; // FI30 -> fraction of top 30 most connected nodes damaged
	int n; // like a counter, keeps track of which sample time is next to be filled
	const int FIcurvelength = 150; // reserve a length of 150 for FI curves (i.e. up to age 150)
	// May need to be changed with very different GNM parameters where individuals live past 150 yr
	std::vector<float> sampleTimes; // when to sample FI

};

FICurve::FICurve(){
	this->FIcurvetot.reserve(FIcurvelength);
	this->FIcurvetot.push_back(0.0);
	this->FIcurve30.reserve(FIcurvelength);
	this->FIcurve30.push_back(0.0);
	this->n = 1; // since we have already set FI for age 0, index to be filled is 1
}

// object to store QALY and FI at important time points for each individual
// in main.cpp, will create a vector of FIandQALYvals objects, where each index is for a different individual
class FIandQALYvals {
public:

	FIandQALYvals();

	// used for debugging
	void printVals() const {
		std::cout << "FItotstart: " << FItotstart << ", FItotend: " << FItotend << ", FItotdeath: " << FItotdeath << std::endl;
		std::cout << "FI30start: " << FI30start << ", FI30end: " << FI30end << ", FI30death: " << FI30death << std::endl;
		std::cout << "QALYtot: " << QALYtot << ", QALY30: " << QALY30 << std::endl;
	}

	void setFItotstart(float FItotstart){this->FItotstart = FItotstart;}
	void setFItotend(float FItotend){this->FItotend = FItotend;}
	void setFItotdeath(float FItotdeath){this->FItotdeath = FItotdeath;}
	void setFI30start(float FI30start){this->FI30start = FI30start;}
	void setFI30end(float FI30end){this->FI30end = FI30end;}
	void setFI30death(float FI30death){this->FI30death = FI30death;}
	void setAllQALY(double QALYtot, double QALY30){
		this->QALYtot = QALYtot;
		this->QALY30 = QALY30;
	}

	float getFItotstart() const {return FItotstart;}
	float getFItotend() const {return FItotend;}
	float getFItotdeath() const {return FItotdeath;}
	float getFI30start() const {return FI30start;}
	float getFI30end() const {return FI30end;}
	float getFI30death() const {return FI30death;}
	float getQALYtot() const {return QALYtot;}
	float getQALY30() const {return QALY30;}


private:
	// attributes (will become columns in FIandQALY file)

	/* Note:
	 * QALY are measured from the end of the disease onwards
	 * If individual does not live past the end of the disease, the QALY measures will be 0.
	 * FIstart/end: initialized to -1.0 and will never be set if individual does not live to the
	 * start/end of the disease. Will be set to "nan" when data is written to file
	 * FIdeath: will always be set
	 */

	double QALYtot;
	double QALY30;
	// FI at the start of the disease
	float FItotstart;
	float FI30start;
	// FI at the end of the disease
	float FItotend;
	float FI30end;
	// FI when the individual died
	float FItotdeath;
	float FI30death;

};

// The reset() method is used to reset values for each individual
// so the constructor is left empty
FIandQALYvals::FIandQALYvals(void) {}

// Namespace to separate variable and methods used for analysis (getting FIcurves, QALY, etc.)
namespace H {

	// holds id's of top 30 most connected nodes in the network
	std::vector<int> top30Nodes;

	// holds the current number of total nodes damaged in the network
	// updated every time a node is damaged
	int deficits_tot;
	// hold the current number of the top 30 most connected nodes damaged
	int deficits30;

	// goes from false to true if individual is too sick to get the disease
	// (not enough undamaged nodes to damage)
	bool tooSick;

	FIandQALYvals FIandQALY;

	// Used for QALY calculations
	double area; // area under (1-FItot) vs age curve, added to as person ages
	double area30; // area under (1-FI30) vs age curve, added to as person ages
	double t_low; // remembers previous time

	FICurve FIcurve;

	// wrapper methods for FICurve
	void fillRemianingFIcurve(){
		/*
		 * After person dies, add -1.0 to FIcurve vector until it has a length of 150
		 * -1.0's will be turned into "nan" when the data is written to the FIcurves file
		 */
		FIcurve.fillRemianingFIcurve();
	}

	void createSampleTimes(float timeStep){
		/*
		 * Times at which to sample the FI to create an FI curve for each individual
		 */
		FIcurve.createSampleTimes(timeStep);
	}

	// current sample time
	float getSampleTime(){return FIcurve.getSampleTime();}

	void sampleFI(int N){
		/*
		 * Sample the current FItot and FI30 and add it the FI curves
		 */
		FIcurve.sampleFI(N, deficits_tot, deficits30);
	}

	void resetFIcurves(){
		FIcurve.resetFIcurves();
	}


	void assignQALY() {
		/*
		 * After the individual dies, QALY is assigned
		 */
		FIandQALY.setAllQALY(area, area30);
	}

	void addArea(double Time, int N) {
		double height = 1.0 - static_cast<double>(deficits_tot)/N;
		double height30 = 1.0 - static_cast<double>(deficits30)/(30.0);
		area += height*(Time - t_low);
		area30 += height30*(Time - t_low);
	}

	void setFIstart(int N){
		/*
		 * When disease starts, sample the FI. Store in FIandQALY object
		 */
		float FItot = static_cast<float>(deficits_tot)/N;
		float FI30 = static_cast<float>(deficits30)/30.0;
		FIandQALY.setFItotstart(FItot);
		FIandQALY.setFI30start(FI30);
		//std::cout << "FItot start: " << FItot << std::endl;
		//std::cout << "FI30 start: " << FI30 << std::endl;
	}

	void setFIend(int N){
		/*
		 * When disease ends, sample the FI. Store in FIandQALY object
		 */
		float FItot = static_cast<float>(deficits_tot)/N;
		float FI30 = static_cast<float>(deficits30)/30.0;
		FIandQALY.setFItotend(FItot);
		FIandQALY.setFI30end(FI30);
		//std::cout << "FItot end: " << FItot << std::endl;
		//std::cout << "FI30 end: " << FI30 << std::endl;
	}

	void setFIdeath(int N){
		/*
		 * When individual dies, sample the FI. Store in FIandQALY object
		 */
		float FItot = static_cast<float>(deficits_tot)/N;
		float FI30 = static_cast<float>(deficits30)/30.0;
		FIandQALY.setFItotdeath(FItot);
		FIandQALY.setFI30death(FI30);
		//std::cout << "FItot death: " << FItot << std::endl;
		//std::cout << "FI30 death: " << FI30 << std::endl;
	}

	void reset(){
		/*
		 * Method called for each new individual. Resets values of variables.
		 */
		deficits_tot = 0;
		deficits30 = 0;
		tooSick = false;

		FIandQALY.setFItotstart(-1.0);
		FIandQALY.setFI30start(-1.0);
		FIandQALY.setFItotend(-1.0);
		FIandQALY.setFI30end(-1.0);
		FIandQALY.setFItotdeath(-1.0);
		FIandQALY.setFI30death(-1.0);

		area = 0;
		area30 = 0;
		t_low = 0;

	}

}

#endif /* HEALTHCALC_H_ */
