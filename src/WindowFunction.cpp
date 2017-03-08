#include "WindowFunction.h"

WindowFunction::WindowFunction(WindowFunctionType w_type, int size): w_type(w_type), size(size) {
	coefficients = new double[size];
	createCoefficients();
}

WindowFunction::~WindowFunction() {
	delete[] coefficients;
}

double WindowFunction::at(int i) {
	return coefficients[i];
}

void WindowFunction::createCoefficients() {
	for (int i = 0; i < size; ++i) {
		coefficients[i] = coeff(i);
	}
}

double WindowFunction::coeff(int n) const {
	double M = (double)size;
	double i = (double)n;

	double alpha;
	double beta;
	double alpha0;
	double alpha1;
	double alpha2;
	double alpha3;
	double alpha4;

	switch (w_type) {
		case Square:
			return 1.0f;
		case VonHann:
			return 0.5f * (1.0f - cos((2.0f * M_PI * i) / (M - 1.0f)));
		case Hamming:
			alpha = 25.0f / 46.0f;
			beta = 1.0 - alpha;
			return alpha - beta * ((2.0f * M_PI * i) / (M - 1.0f));
		case Blackman:
			alpha = 0.16f;
			alpha0 = (1.0f - alpha) / 2.0f;
			alpha1 = 1.0f / 2.0f;
			alpha2 = alpha / 2.0f;
			return alpha0 - alpha1 * cos(((2.0f * M_PI * i) / (M - 1.0f))) + alpha2 * cos((4.0f * M_PI * i) / (M - 1.0f));
		case BlackmanHarris:
			alpha0 = 0.35875f;
			alpha1 = 0.48829f;
			alpha2 = 0.14128f;
			alpha3 = 0.01168f;
			return alpha0 - alpha1 * cos(((2.0f * M_PI * i) / (M - 1.0f))) + alpha2 * cos((4.0f * M_PI * i) / (M - 1.0f)) - alpha3 * cos((6.0f * M_PI * i) / (M - 1.0f));
		case BlackmanNuttall:
			alpha0 = 0.3635819f;
			alpha1 = 0.4891775f;
			alpha2 = 0.1365995f;
			alpha3 = 0.0106411f;
			return alpha0 - alpha1 * cos(((2.0f * M_PI * i) / (M - 1.0f))) + alpha2 * cos((4.0f * M_PI * i) / (M - 1.0f)) - alpha3 * cos((6.0f * M_PI * i) / (M - 1.0f));
		case FlatTop:
			alpha0 = 1.0f;
			alpha1 = 1.93f;
			alpha2 = 1.29f;
			alpha3 = 0.388f;
			alpha4 = 0.028f;
			return alpha0 - alpha1 * cos(((2.0f * M_PI * i) / (M - 1.0f))) + alpha2 * cos((4.0f * M_PI * i) / (M - 1.0f)) - alpha3 * cos((6.0f * M_PI * i) / (M - 1.0f)) + alpha4 * cos((8.0f * M_PI * i) / (M - 1.0f));
	}
	return 1.0f;
}