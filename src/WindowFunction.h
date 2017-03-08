/**
 * @author Martin Mende https://github.com/mmende
 */

#include <cmath>

/**
 * The possible window function types.
 */
enum WindowFunctionType
{
	Square,
	VonHann,
	Hamming,
	Blackman,
	BlackmanHarris,
	BlackmanNuttall,
	FlatTop
};

/**
 * Creates the window function coefficients for a N-window.
 */
class WindowFunction {
public:
	WindowFunction(WindowFunctionType w_type, int size);
	~WindowFunction();

	/**
	 * Returns the coefficient at index i.
	 *
	 * @param  i The index.
	 *
	 * @return   The coefficient.
	 */
	double at(int i);
private:
	/** Specifies which type of window function will be used. */
	WindowFunctionType w_type;

	/** The size of the windows. */
	int size;

	/** Stores the coefficients. */
	double* coefficients;

	/** Computes the coefficients. */
	void createCoefficients();

	/** Returns a coefficient for a specific index. */
	double coeff(int n) const;
};