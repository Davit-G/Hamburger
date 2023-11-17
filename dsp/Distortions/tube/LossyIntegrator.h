#include <JuceHeader.h>

class LossyIntegrator
{
public:
	LossyIntegrator() {}
	~LossyIntegrator() {}

	/** reset members to initialized state */
	bool prepareToPlay(double _sampleRate)
	{
		sampleRate = _sampleRate;
		integrator_z[0] = 0.0;
		integrator_z[1] = 0.0;

		return true;
	}
    
	double processAudioSample(double xn)
	{
		double hpf = alpha0*(xn - rho*integrator_z[0] - integrator_z[1]);
		double bpf = alpha*hpf + integrator_z[0];
		double lpf = alpha*bpf + integrator_z[1];
		double bsf = hpf + lpf;

		integrator_z[0] = alpha*hpf + bpf;
		integrator_z[1] = alpha*bpf + lpf;

        return lpf;
	}

	void calculateFilterCoeffs()
	{
		double fc = 5.0;
		double Q = 0.707;

		double wd = juce::MathConstants<double>::twoPi * fc;
		double T = 1.0 / sampleRate;
		double wa = (2.0 / T)*tan(wd*T / 2.0);
		double g = wa*T / 2.0;
		
        double R = 1.0 / (2.0*Q);
        alpha0 = 1.0 / (1.0 + 2.0*R*g + g*g);
        alpha = g;
        rho = 2.0*R + g;
	}

protected:
	double sampleRate = 44100.0;				///< current sample rate

	double integrator_z[2];						///< state variables

	double alpha0 = 0.0;		///< input scalar, correct delay-free loop
	double alpha = 0.0;			///< alpha is (wcT/2)
	double rho = 0.0;			///< p = 2R + g (feedback)

	double beta = 0.0;			///< beta value, not used

	// --- for analog Nyquist matching
	double analogMatchSigma = 0.0; ///< analog matching Sigma value (see book)
};