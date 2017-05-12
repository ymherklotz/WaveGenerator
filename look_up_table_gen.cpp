#include <cmath>
#include <iostream>

int main()
{
	const int SAMPLES=16;

	std::cout<<"int sine_table["<<SAMPLES<<"]={";
	
	for(int i=0; i<SAMPLES; ++i)
	{
		if(i==0)
			std::cout<<(int)(0.5*pow(2, 16))<<",";
		else
			std::cout<<(int)(((1+std::sin((long double)i*(long double)2*M_PIl/(long double)SAMPLES))/2)*(pow(2, 16)-1))<<",";
	}

	std::cout<<"};\nint triangle_table["<<SAMPLES<<"]={";

	for(int i=0; i<SAMPLES; ++i)
	{
		if(i<SAMPLES/2)
		{
			std::cout<<(int)((long double)i/(long double)SAMPLES*2*(pow(2, 16)-1))<<",";
		}
		else
		{
			std::cout<<(int)((long double)(SAMPLES-i)/(long double)SAMPLES*2*(pow(2, 16)-1))<<",";
		}
	}

	std::cout<<"};\n";
	
	return 0;
}
