
#include "interpreter.h"
#include "lexer.cpp"
#include "parser.cpp"

#include <cstdarg>

Interpreter::stm_ptr Interpreter::op_special[NUM_STAT] = {
	&Interpreter::Execute_Print
};

Interpreter::Interpreter()
{
	interpreter	= this;
	bytecode	= 0;
	bytesize	= 0;
	heap		= 0;
	heapsize	= 0;
}

Interpreter::~Interpreter()
{
    if (bytecode) {
		free(bytecode);

		bytecode = 0;
		bytesize = 0;
	}

	if (heap) {
		free(heap);
		heapsize = 0;
	}
}

void Interpreter::AddCodeEntry(unsigned char opcode, void* arg1, void* arg2)
{
	INTERP_NERROR(, "Interpreter::AddCodeEntry(): Out of memory", (bytesize + ENTRY_SIZE) >= CODE_SIZE);

	char* ptr = (bytecode + bytesize);

	*((unsigned char*)ptr) = opcode;
	*((void**)(ptr + 1)) = arg1;
	*((void**)(ptr + 1 + sizeof(void*))) = arg2;

	bytesize += ENTRY_SIZE;
}

void Interpreter::Cleanup()
{
	for (garbagelist::iterator it = garbage.begin(); it != garbage.end(); ++it)
		delete (*it);

	garbage.clear();
}

bool Interpreter::Compile(const std::string& file)
{
#ifdef _MSC_VER
	FILE* infile = NULL;
	fopen_s(&infile, file.c_str(), "rb");
#else
	FILE* infile = fopen(file.c_str(), "rb");
#endif

	INTERP_ERROR(false, "Interpreter::Compile(): Could not open file", infile);

	// get file length
	fseek(infile, 0, SEEK_END);
	long end = ftell(infile);

	fseek(infile, 0, SEEK_SET);
	long length = end - ftell(infile);

	// read program into buffer
	char* buffer = (char*)malloc((length + 2) * sizeof(char));
	INTERP_ERROR(false, "Interpreter::Compile(): Could not create buffer", buffer);

	buffer[length + 1] = buffer[length] = 0;

	fread(buffer, sizeof(char), length, infile);
	fclose(infile);

	std::cout << "Compiling \'" << file << "\'\n";
	yy_scan_buffer(buffer, length + 2);

	if (!bytecode)
		bytecode = (char*)malloc(CODE_SIZE);

	if (!heap) {
		heapsize = 0;
		heap = (char*)malloc(HEAP_SIZE);
	}

	bytesize = 0;
	progname = file;

	// run lexer and parser
	int ret = yyparse();

	yy_delete_buffer(YY_CURRENT_BUFFER);
	free(buffer);

	Cleanup();

	INTERP_NERROR(false, "Interpreter::Compile(): Parser error", ret != 0);
	return true;
}

bool Interpreter::Run()
{
	std::cout << "Executing program '" << progname << "'...\n";
	INTERP_ERROR(false, "Interpreter::Run(): No code generated", bytecode);

	size_t		off = 0;
	stm_ptr		stm;
	uint8_t		opcode;
	void *arg1, *arg2;
	char* ptr;

	while (off != bytesize) {
		ptr = (bytecode + off);

		opcode = *((unsigned char*)ptr);
		arg1 = *((void**)(ptr + 1));
		arg2 = *((void**)(ptr + 1 + sizeof(void*)));

		off += ENTRY_SIZE;

		// 20 special statements reserved
		if (opcode < 0x20) {
			stm = op_special[opcode];
			(*stm)(arg1, arg2);
		} else {
			// TODO: common statements
		}
	}

	return true;
}
