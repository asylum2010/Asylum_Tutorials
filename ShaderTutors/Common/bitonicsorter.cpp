
#include "bitonicsorter.h"

BitonicSorter::BitonicSorter(const char* csfile, uint32_t workgroupsize)
{
	char defines[128];
	sprintf_s(defines, "#define THREADGROUP_SIZE	%d\n#define LOG2_THREADGROUP_SIZE	%d\n\n", workgroupsize, Math::Log2OfPow2(workgroupsize));

	GLCreateComputeProgramFromFile(csfile, &computeshader, defines);
	threadgroupsize = workgroupsize;
}

BitonicSorter::~BitonicSorter()
{
	delete computeshader;
}

void BitonicSorter::Sort(GLuint buffer, int count, bool ascending)
{
	// NOTE: optimized version

	int pow2count = Math::NextPow2(count);
	int log2threadgroupsize = Math::Log2OfPow2(threadgroupsize);
	int log2count = Math::Log2OfPow2(pow2count);

	GLuint funcindex = 0;
	GLuint numthreadgroups = pow2count / threadgroupsize;

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);

	computeshader->SetInt("numValues", count);
	computeshader->SetInt("ascending", (ascending ? 1 : 0));
	computeshader->Begin();

	// presort up to k = threadgroupsize
	glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &funcindex);
	glDispatchCompute(numthreadgroups, 1, 1);

	// must synchronize incoherent writes
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// progressive sort from k = 2 * threadgroupsize
	for (int s = log2threadgroupsize + 1; s <= log2count; ++s) {
		computeshader->SetInt("outerIndex", s);

		for (int t = s - 1; t >= log2threadgroupsize; --t) {
			funcindex = 1;

			computeshader->SetInt("innerIndex", t);
			computeshader->CommitChanges();

			glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &funcindex);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
			glDispatchCompute(numthreadgroups, 1, 1);

			// must synchronize incoherent writes
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		// we can sort this part more efficiently
		funcindex = 2;

		glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &funcindex);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
		glDispatchCompute(numthreadgroups, 1, 1);

		// must synchronize incoherent writes
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	computeshader->End();
}

void BitonicSorter::TEST_Presort(GLuint buffer, int count, bool ascending)
{
	GLuint funcindex = 0;
	int pow2count = Math::NextPow2(count);

	computeshader->SetInt("numValues", count);
	computeshader->SetInt("ascending", (ascending ? 1 : 0));
	computeshader->Begin();
	{
		glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &funcindex);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
		glDispatchCompute(pow2count / threadgroupsize, 1, 1);
	}
	computeshader->End();

	// must synchronize incoherent writes
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void BitonicSorter::TEST_Progressive(GLuint buffer, int count, bool ascending)
{
	GLuint funcindex;
	int log2threadgroupsize = Math::Log2OfPow2(threadgroupsize);
	int pow2count = Math::NextPow2(count);
	int log2count = Math::Log2OfPow2(pow2count);

	computeshader->SetInt("ascending", (ascending ? 1 : 0));

	for (int s = log2threadgroupsize + 1; s <= log2count; ++s) {
		for (int t = s - 1; t >= log2threadgroupsize; --t) {
			funcindex = 1;

			computeshader->SetInt("outerIndex", s);
			computeshader->SetInt("innerIndex", t);
			computeshader->Begin();
			{
				glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &funcindex);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
				glDispatchCompute(pow2count / threadgroupsize, 1, 1);
			}
			computeshader->End();

			// must synchronize incoherent writes
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		// we can sort this part more effectively
		funcindex = 2;

		computeshader->SetInt("outerIndex", s);
		computeshader->Begin();
		{
			glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &funcindex);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
			glDispatchCompute(pow2count / threadgroupsize, 1, 1);
		}
		computeshader->End();

		// must synchronize incoherent writes
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
}
