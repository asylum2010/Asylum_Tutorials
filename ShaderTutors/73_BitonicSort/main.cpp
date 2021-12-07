
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>
#include <chrono>
#include <random>

#include "..\Common\application.h"
#include "..\Common\bitonicsorter.h"

constexpr int log2_N = 5;
constexpr int N = (1 << log2_N);

std::mt19937	gen;
GLuint			valuebuffer		= 0;
int*			hostvalues		= nullptr;
int*			mappedvalues	= nullptr;

static void GenerateRandom()
{
	std::uniform_int_distribution<> uniform;

	for (int i = 0; i < N; ++i) {
		hostvalues[i] = uniform(gen) % 100;
		mappedvalues[i] = hostvalues[i];
	}
}

static void WriteOut(int* source, int k)
{
	for (int i = 0; i < N; ++i)
		std::cout << source[i] << " ";

	//std::cout << " (" << k << ")" << std::endl;
	std::cout << std::endl;
}

static void VerifyEqual()
{
	bool isequal = true;

	for (int i = 0; i < N; ++i) {
		if (hostvalues[i] != mappedvalues[i]) {
			isequal = false;
			break;
		}
	}

	std::cout << (isequal ? "EQUAL" : "NOT EQUAL") << std::endl;
	assert(isequal);
}

static void VerifySorted(bool ascending)
{
	int nullelem = (ascending ? -INT_MAX : INT_MAX);
	int current = nullelem;

	for (int i = 0; i < N; ++i) {
		if (ascending ? (hostvalues[i] < current) : (hostvalues[i] > current)) {
			current = nullelem;
			break;
		} else {
			current = hostvalues[i];
		}
	}

	if (ascending)
		std::cout << ((current != nullelem) ? "SORTED (ASCENDING)" : "NOT ASCENDING") << std::endl;
	else
		std::cout << ((current != nullelem) ? "SORTED (DESCENDING)" : "NOT DESCENDING") << std::endl;

	assert(current != nullelem);
}

void Test_Bitonic_Sort(BitonicSorter& sorter, bool ascending, bool optimized)
{
	GLuint funcindex = 0;
	int threadgroupsize = sorter.GetThreadGroupSize();
	int log2threadgroupsize = Math::Log2OfPow2(threadgroupsize);

	std::cout << "\nGenerated series:\n";
	WriteOut(hostvalues, 0);

	if (optimized) {
		// test optimized sort
		std::cout << "\nOptimized method:\n";

		// host side
		for (int s = 1; s <= log2_N; ++s) {
			for (int t = s - 1; t >= 0; --t) {
				for (int l = 0; l < N; ++l) {
					int lxt = l ^ (1 << t);
					bool dir = ((l & (1 << s)) == 0);

					if (lxt > l) {
						if ((hostvalues[l] < hostvalues[lxt]) == (dir != ascending))
							std::swap(hostvalues[l], hostvalues[lxt]);
					}
				}
			}
		}

		// device side
		sorter.Sort(valuebuffer, N, ascending);
		glFinish();

		WriteOut(hostvalues, 1);
		WriteOut(mappedvalues, 1);

		VerifyEqual();
		VerifySorted(ascending);
	} else {
		// presort up to k = THREADGROUP_SIZE
		std::cout << "\nBitonic presort up to k = " << threadgroupsize << ":\n";
	
		// host side
		for (int s = 1; s <= log2threadgroupsize; ++s) {
			for (int t = s - 1; t >= 0; --t) {
				for (int l = 0; l < N; ++l) {
					int lxt = l ^ (1 << t);
					bool dir = ((l & (1 << s)) == 0);

					if (lxt > l) {
						if ((hostvalues[l] < hostvalues[lxt]) == (dir != ascending))
							std::swap(hostvalues[l], hostvalues[lxt]);
					}
				}
			}
		}

		// device side
		sorter.TEST_Presort(valuebuffer, N, ascending);
		glFinish();

		WriteOut(hostvalues, 1);
		WriteOut(mappedvalues, 1);
		VerifyEqual();

		// continue sorting
		if (log2threadgroupsize + 1 <= log2_N)
			std::cout << "\nProgressive sort from k = " << (threadgroupsize << 1) << " to k = " << N << ":\n";
		else
			std::cout << "\nProgressive sort skipped\n";

		// host side
		for (int s = log2threadgroupsize + 1; s <= log2_N; ++s) {
			for (int t = s - 1; t >= 0; --t) {
				for (int l = 0; l < N; ++l) {
					int lxt = l ^ (1 << t);
					bool dir = ((l & (1 << s)) == 0);

					if (lxt > l) {
						if ((hostvalues[l] < hostvalues[lxt]) == (dir != ascending))
							std::swap(hostvalues[l], hostvalues[lxt]);
					}
				}
			}
		}

		// device side
		sorter.TEST_Progressive(valuebuffer, N, ascending);
		glFinish();

		WriteOut(hostvalues, 1);
		WriteOut(mappedvalues, 1);

		VerifyEqual();
		VerifySorted(ascending);
	}

	std::cout << std::endl;
}

int main(int argc, char* argv[])
{
	Application* app = Application::Create(100, 100);

	if (!app->InitializeDriverInterface(GraphicsAPIOpenGL)) {
		delete app;
		return 1;
	}

	gen.seed((unsigned int)time(0));

	{
		BitonicSorter sorters[] = {
			BitonicSorter("../../Media/ShadersGL/bitonicsort.comp", 4),
			BitonicSorter("../../Media/ShadersGL/bitonicsort.comp", 8),
			BitonicSorter("../../Media/ShadersGL/bitonicsort.comp", 16),
			BitonicSorter("../../Media/ShadersGL/bitonicsort.comp", 32)
		};

		// create buffer
		glGenBuffers(1, &valuebuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, valuebuffer);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, N * sizeof(int), hostvalues, GL_MAP_READ_BIT|GL_MAP_WRITE_BIT|GL_MAP_COHERENT_BIT|GL_MAP_PERSISTENT_BIT);

		hostvalues = new int[N];
		mappedvalues = (int*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

		// test
		for (auto& sorter : sorters) {
			std::cout << "\nTHREADGROUP_SIZE: " << sorter.GetThreadGroupSize() << std::endl;

			GenerateRandom();
			Test_Bitonic_Sort(sorter, false, false);

			GenerateRandom();
			Test_Bitonic_Sort(sorter, true, false);

			GenerateRandom();
			Test_Bitonic_Sort(sorter, false, true);

			GenerateRandom();
			Test_Bitonic_Sort(sorter, true, true);
		}
	}

	// clean up
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glDeleteBuffers(1, &valuebuffer);

	delete[] hostvalues;
	delete app;

	return 0;
}
