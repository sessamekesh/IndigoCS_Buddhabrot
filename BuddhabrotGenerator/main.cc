#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
#include <vector>
#include <sstream>
using namespace std;

typedef unsigned int HeatmapType;

class Complex
{
public:
	Complex(double r = 0.0, double i = 0.0)
		: _r(r), _i(i)
	{ }

	Complex(const Complex&) = default;

	double r() const { return _r; }
	double i() const { return _i; }

	Complex operator*(const Complex& other)
	{
		// (a + bi) (c + di)
		return Complex(_r * other._r - _i * other._i, _r * other._i + _i * other._r);
	}

	Complex operator+(const Complex& other)
	{
		return Complex(_r + other._r, _i + other._i);
	}

	double sqmagnitude() const
	{
		return _r * _r + _i * _i;
	}

private:
	double _r, _i;
};

//
// Utility
//
void AllocHeatmap(HeatmapType**& o_heatmap, int width, int height)
{
	o_heatmap = new HeatmapType*[height];
	for (int i = 0; i < height; ++i)
	{
		o_heatmap[i] = new HeatmapType[width];
		for (int j = 0; j < width; ++j)
		{
			o_heatmap[i][j] = 0;
		}
	}
}

void FreeHeatmap(HeatmapType**& o_heatmap, int height)
{
	for (int i = 0; i < height; ++i)
	{
		delete[] o_heatmap[i];
		o_heatmap[i] = nullptr;
	}
	delete o_heatmap;
	o_heatmap = nullptr;
}

vector<Complex> buddhabrotPoints(const Complex& c, int nIterations)
{
	int n = 0;
	Complex z;

	vector<Complex> toReturn;
	toReturn.reserve(nIterations);

	while (n < nIterations && z.sqmagnitude() <= 2.0)
	{
		z = z * z + c;
		++n;

		toReturn.push_back(z);
	}

	// If point remains bounded through nIterations iterations, the point
	//  is bounded, therefore in the Mandelbrot set, therefore of no interest to us
	if (n == nIterations)
	{
		return vector<Complex>();
	}
	else
	{
		return toReturn;
	}
}

int rowFromReal(double real, double minR, double maxR, int imageHeight)
{
	// [minR, maxR]
	// [0, maxR - minR] // subtract minR from n
	// [0, imageHeight] // multiply by (imageHeight / (maxR - minR))
	return static_cast<int>((real - minR) * (imageHeight / (maxR - minR)));
}

int colFromImaginary(double imag, double minI, double maxI, int imageWidth)
{
	return static_cast<int>((imag - minI) * (imageWidth / (maxI - minI)));
}

void GenerateHeatmap(HeatmapType** o_heatmap, int imageWidth, int imageHeight,
	const Complex& minimum, const Complex& maximum, int nIterations, long long nSamples,
	HeatmapType& o_maxHeatmapValue, string consoleMessagePrefix)
{
	mt19937 rng;
	uniform_real_distribution<double> realDistribution(minimum.r(), maximum.r());
	uniform_real_distribution<double> imagDistribution(minimum.i(), maximum.i());

	rng.seed(chrono::high_resolution_clock::now().time_since_epoch().count());

	auto next = chrono::high_resolution_clock::now() + chrono::seconds(5);

	// Collect nSamples samples... (sample is just a random number c)
	for (long long sampleIdx = 0; sampleIdx < nSamples; ++sampleIdx)
	{
		if (chrono::high_resolution_clock::now() > next)
		{
			next = chrono::high_resolution_clock::now() + chrono::seconds(30);
			cout << consoleMessagePrefix << "Samples Taken: " << sampleIdx << "/" << nSamples << endl;
		}

		//  Each sample, get the list of points as the function
		//    escapes to infinity (if it does at all)
		
		Complex sample(realDistribution(rng), imagDistribution(rng));
		vector<Complex> pointsList = buddhabrotPoints(sample, nIterations);

		for (Complex& point : pointsList)
		{
			if (point.r() <= maximum.r() && point.r() >= minimum.r()
				&& point.i() <= maximum.i() && point.i() >= minimum.i())
			{
				int row = rowFromReal(point.r(), minimum.r(), maximum.r(), imageHeight);
				int col = colFromImaginary(point.i(), minimum.i(), maximum.i(), imageWidth);
				++o_heatmap[row][col];

				if (o_heatmap[row][col] > o_maxHeatmapValue)
				{
					o_maxHeatmapValue = o_heatmap[row][col];
				}
			}
		}
	}
}

int colorFromHeatmap(HeatmapType inputValue, HeatmapType maxHeatmapValue, int maxColor)
{
	double scale = static_cast<double>(maxColor) / maxHeatmapValue;

	return inputValue * scale;
}

string elapsedTime(chrono::nanoseconds elapsedTime)
{
	chrono::hours hrs = chrono::duration_cast<chrono::hours>(elapsedTime);
	chrono::minutes mins = chrono::duration_cast<chrono::minutes>(elapsedTime - hrs);
	chrono::seconds secs = chrono::duration_cast<chrono::seconds>(elapsedTime - hrs - mins);
	chrono::milliseconds mils = chrono::duration_cast<chrono::milliseconds>(elapsedTime - hrs - mins - secs);

	stringstream ss("");

	if (hrs.count() > 24)
	{
		ss << hrs.count() / 24 << " Days, " << hrs.count() % 24 << " Hours, ";
	}
	else if (hrs.count() > 0)
	{
		ss << hrs.count() << " Hours, ";
	}

	if (mins.count() > 0)
	{
		ss << mins.count() << " Minutes, ";
	}

	if (secs.count() > 0)
	{
		ss << secs.count() << " Seconds, ";
	}

	if (mils.count() > 0)
	{
		ss << mils.count() << " Milliseconds";
	}

	return ss.str();
}

int main()
{
	const Complex MINIMUM(-2.0, -2.0);
	const Complex MAXIMUM(1.0, 2.0);
	const int IMAGE_HEIGHT = 7000;
	const int IMAGE_WIDTH = 7000;

	const int RED_ITERS = 5;
	const int BLUE_ITERS = 500000;
	const int GREEN_ITERS = 500;

	const long long int SAMPLE_COUNT = IMAGE_WIDTH * IMAGE_HEIGHT * 350ll;

	auto startTime = chrono::high_resolution_clock::now();

	ofstream imgOut("out.ppm");
	if (!imgOut)
	{
		cout << "Could not open image file for writing!" << endl;
		cout << "Press ENTER to continue..." << endl;
		cin.ignore();
		return EXIT_FAILURE;
	}

	// Allocate a heatmap of the size of our image
	HeatmapType maxHeatmapValue = 0;
	HeatmapType** red;
	HeatmapType** green;
	HeatmapType** blue;
	AllocHeatmap(red, IMAGE_WIDTH, IMAGE_HEIGHT);
	AllocHeatmap(green, IMAGE_WIDTH, IMAGE_HEIGHT);
	AllocHeatmap(blue, IMAGE_WIDTH, IMAGE_HEIGHT);

	// Generate heatmap
	GenerateHeatmap(red, IMAGE_WIDTH, IMAGE_HEIGHT, MINIMUM, MAXIMUM, RED_ITERS,
		SAMPLE_COUNT, maxHeatmapValue, "Red Channel: ");
	GenerateHeatmap(green, IMAGE_WIDTH, IMAGE_HEIGHT, MINIMUM, MAXIMUM, GREEN_ITERS,
		SAMPLE_COUNT, maxHeatmapValue, "Green Channel: ");
	GenerateHeatmap(blue, IMAGE_WIDTH, IMAGE_HEIGHT, MINIMUM, MAXIMUM, BLUE_ITERS,
		SAMPLE_COUNT, maxHeatmapValue, "Blue Channel: ");

	// Scale the heatmap down
	for (int row = 0; row < IMAGE_HEIGHT; ++row)
	{
		for (int col = 0; col < IMAGE_WIDTH; ++col)
		{
			red[row][col] = colorFromHeatmap(red[row][col], maxHeatmapValue, 255);
			green[row][col] = colorFromHeatmap(green[row][col], maxHeatmapValue, 255);
			blue[row][col] = colorFromHeatmap(blue[row][col], maxHeatmapValue, 255);
		}
	}

	// Write PPM header
	imgOut << "P3" << endl;
	imgOut << IMAGE_WIDTH << " " << IMAGE_HEIGHT << endl;
	imgOut << 255 << endl;

	// Write PPM image from color maps
	for (int row = 0; row < IMAGE_HEIGHT; ++row)
	{
		for (int col = 0; col < IMAGE_WIDTH; ++col)
		{
			imgOut << red[row][col] << " ";
			imgOut << green[row][col] << " ";
			imgOut << blue[row][col] << "   ";
		}
		imgOut << endl;
	}

	FreeHeatmap(red, IMAGE_HEIGHT);
	FreeHeatmap(green, IMAGE_HEIGHT);
	FreeHeatmap(blue, IMAGE_HEIGHT);

	auto endPoint = chrono::high_resolution_clock::now();

	cout << "Time elapsed: " << elapsedTime(endPoint - startTime) << endl;
	cout << "Finished generating image. Open in GIMP to view. Press ENTER to exit." << endl;
	cin.ignore();

	return EXIT_SUCCESS;
}