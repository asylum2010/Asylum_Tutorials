
#ifndef _BITONICSORTER_H_
#define _BITONICSORTER_H_

#include "gl4ext.h"

/**
 * \brief Parallel bitonic sorter for OpenGL
 *
 * Expects a compute shader with 3 subroutines with the signature
 *     void (uint, uint)
 * the following uniform variables:
 *
 * int numValues;		// the count of (valid) values to sort
 * int outerIndex;		// outer loop index of the algorithm
 * int innerIndex;		// inner loop index
 * int ascending;		// sort ascending (1) or descending (0)
 *
 * and a read-write shader storage buffer at index 0.
 */
class BitonicSorter
{
private:
	OpenGLEffect* computeshader;
	uint32_t threadgroupsize;

public:
	BitonicSorter(const char* csfile, uint32_t workgroupsize);
	~BitonicSorter();

	void Sort(GLuint buffer, int count, bool ascending);

	// utility methods for testing
	void TEST_Presort(GLuint buffer, int count, bool ascending);
	void TEST_Progressive(GLuint buffer, int count, bool ascending);

	inline uint32_t GetThreadGroupSize() const	{ return threadgroupsize; }
	inline OpenGLEffect* GetShader() const		{ return computeshader; }
};

#endif
