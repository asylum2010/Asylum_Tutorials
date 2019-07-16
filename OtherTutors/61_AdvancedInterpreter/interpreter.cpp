
#include "interpreter.h"
#include "lexer.cpp"
#include "parser.cpp"

#include <cstdarg>

Interpreter::stm_ptr Interpreter::op_special[NUM_SPECIAL] = {
	&Interpreter::Print_Reg,
	&Interpreter::Print_Memory
};

Interpreter::Interpreter()
{
	interpreter = this;
	stack = nullptr;

	scopes.resize(5);
}

Interpreter::~Interpreter()
{
	if (stack != nullptr) {
		free(stack);
		stack = 0;
	}
}

void Interpreter::Cleanup()
{
	for (size_t i = 0; i < scopes.size(); ++i)
		scopes[i].clear();

	scopes.clear();
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

	if (stack == nullptr)
		stack = (char*)malloc(STACK_SIZE);

	progname = file;
	current_scope = 0;
	current_func = 0;
	alloc_addr = 0;

	// run lexer and parser
	int ret = yyparse();

	yy_delete_buffer(YY_CURRENT_BUFFER);
	free(buffer);

	Cleanup();

	INTERP_NERROR(false, "Interpreter::Compile(): Parser error", ret != 0);
	return true;
}

bool Interpreter::Link()
{
	size_t	off = 0;
	size_t	bytesize = program.Size();
	int64_t	arg1;

	char*	bytecode = program.Data();
	char*	ptr;
	
	uint8_t opcode;
	unresolved_reference* ref;
	ByteStream tmp;
	std::set<symbol_desc*> junk;

	while (off < bytesize) {
		ptr = (bytecode + off);
		off += ENTRY_SIZE;

		opcode = *((uint8_t*)ptr);
		arg1 = ARG1_INT(ptr);

		if (opcode == OP_JMP && arg1 == UNKNOWN_ADDR) {
			ref = reinterpret_cast<unresolved_reference*>(ARG2_PTR(ptr));
			INTERP_ERROR(false, "Linker internal error", ref);

			if (ref->func) {
				INTERP_NERROR(false, "Unresolved external '" << ref->func->name << "'", ref->func->address == UNKNOWN_ADDR);

				tmp << OP(OP_JMP) << (ref->func->address - (int)off) << NIL;
				memcpy(ptr, tmp.Data(), tmp.Size());

				junk.insert(ref->func);
			} else {
				// nothing for now
			}

			tmp.Clear();
			Deallocate(ref);
		}
	}

	for (auto it = junk.begin(); it != junk.end(); ++it)
		Deallocate(*it);

	return true;
}

bool Interpreter::Run()
{
	if (program.Size() == 0)
		return false;

	stm_ptr stm;
	uint8_t opcode;
	char* ptr;

	memset(registers, 0, sizeof(registers));

	registers[EBP] = STACK_SIZE;
	registers[ESP] = STACK_SIZE;
	registers[EIP] = entry;

	char* bytecode = program.Data();
	size_t bytesize = program.Size();
	size_t stackdepth = 0;

	std::cout << "Executing program '" << progname << "'...\n";

	while ((size_t)registers[EIP] < bytesize) {
		ptr = (bytecode + registers[EIP]);

		opcode = *((uint8_t*)ptr);
		registers[EIP] += ENTRY_SIZE;

		// 20 special statements reserved
		if (opcode < 0x20) {
			stm = op_special[opcode];
			(*stm)(ARG1_PTR(ptr), ARG2_PTR(ptr));
		} else {
			// common statements
			switch (opcode) {
			case OP_PUSH: {
				int64_t& esp = registers[ESP];

				esp -= 8;
				*((int64_t*)(stack + esp)) = registers[ARG1_INT(ptr)];

				if (esp < 0)
					INTERP_NERROR(false, "EXCEPTION: Stack overflow", true);

				++stackdepth;
				} break;

			case OP_PUSHADD: {
				int64_t& esp = registers[ESP];

				esp -= 8;
				*((int64_t*)(stack + esp)) = registers[ARG1_INT(ptr)] + ARG2_INT(ptr);

				if (esp < 0)
					INTERP_NERROR(false, "EXCEPTION: Stack overflow", true);

				++stackdepth;
				} break;

			case OP_POP: {
				int64_t& esp = registers[ESP];

				registers[ARG1_INT(ptr)] = *((int64_t*)(stack + esp));
				esp += 8;
				
				INTERP_NERROR(false, "EXCEPTION: Stack underflow", stackdepth == 0);
				--stackdepth;
				} break;

			case OP_MOV_RS:
				registers[ARG1_INT(ptr)] = ARG2_INT(ptr);
				break;

			case OP_MOV_RR:
				registers[ARG1_INT(ptr)] = registers[ARG2_INT(ptr)];
				break;

			case OP_MOV_RM:
				registers[ARG1_INT(ptr)] = STACK_INT(registers[EBP] + ARG2_INT(ptr));
				break;

			case OP_MOV_MR:
				STACK_INT(registers[EBP] + ARG1_INT(ptr)) = registers[ARG2_INT(ptr)];
				break;

			case OP_MOV_MM:
				STACK_INT(registers[EBP] + ARG1_INT(ptr)) = STACK_INT(registers[EBP] + ARG2_INT(ptr));
				break;

			case OP_AND_RS:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] && ARG2_INT(ptr));
				break;

			case OP_AND_RR:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] && registers[ARG2_INT(ptr)]);
				break;

			case OP_OR_RS:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] || ARG2_INT(ptr));
				break;

			case OP_OR_RR:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] || registers[ARG2_INT(ptr)]);
				break;

			case OP_NOT:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] == 0);
				break;

			case OP_ADD_RS:
				registers[ARG1_INT(ptr)] += ARG2_INT(ptr);
				break;

			case OP_ADD_RR:
				registers[ARG1_INT(ptr)] += registers[ARG2_INT(ptr)];
				break;

			case OP_SUB_RS:
				registers[ARG1_INT(ptr)] -= ARG2_INT(ptr);
				break;
			
			case OP_SUB_RR:
				registers[ARG1_INT(ptr)] -= registers[ARG2_INT(ptr)];
				break;

			case OP_MUL_RS:
				registers[ARG1_INT(ptr)] *= ARG2_INT(ptr);
				break;

			case OP_MUL_RR:
				registers[ARG1_INT(ptr)] *= registers[ARG2_INT(ptr)];
				break;

			case OP_DIV_RS:
				if( ARG2_INT(ptr) == 0 )
					INTERP_NERROR(false, "EXCEPTION: Division by zero", true);

				registers[ARG1_INT(ptr)] /= ARG2_INT(ptr);
				break;

			case OP_DIV_RR:
				if( registers[ARG2_INT(ptr)] == 0 )
					INTERP_NERROR(false, "EXCEPTION: Division by zero", true);

				registers[ARG1_INT(ptr)] /= registers[ARG2_INT(ptr)];
				break;

			case OP_MOD_RS:
				registers[ARG1_INT(ptr)] %= ARG2_INT(ptr);
				break;

			case OP_MOD_RR:
				registers[ARG1_INT(ptr)] %= registers[ARG2_INT(ptr)];
				break;

			case OP_NEG:
				registers[ARG1_INT(ptr)] = -registers[ARG1_INT(ptr)];
				break;

			case OP_SETL_RS:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] < ARG2_INT(ptr));
				break;

			case OP_SETL_RR:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] < registers[ARG2_INT(ptr)]);
				break;

			case OP_SETLE_RS:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] <= ARG2_INT(ptr));
				break;

			case OP_SETLE_RR:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] <= registers[ARG2_INT(ptr)]);
				break;

			case OP_SETG_RS:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] > ARG2_INT(ptr));
				break;

			case OP_SETG_RR:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] > registers[ARG2_INT(ptr)]);
				break;

			case OP_SETGE_RS:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] >= ARG2_INT(ptr));
				break;

			case OP_SETGE_RR:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] >= registers[ARG2_INT(ptr)]);
				break;

			case OP_SETE_RS:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] == ARG2_INT(ptr));
				break;

			case OP_SETE_RR:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] == registers[ARG2_INT(ptr)]);
				break;

			case OP_SETNE_RS:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] != ARG2_INT(ptr));
				break;

			case OP_SETNE_RR:
				registers[ARG1_INT(ptr)] = (registers[ARG1_INT(ptr)] != registers[ARG2_INT(ptr)]);
				break;

			case OP_JZ:
				if (registers[ARG1_INT(ptr)] == 0)
					registers[EIP] += ARG2_INT(ptr);

				break;

			case OP_JNZ:
				if (registers[ARG2_INT(ptr)] == 0)
					registers[EIP] += ARG2_INT(ptr);

				break;

			case OP_JMP:
				registers[EIP] += ARG1_INT(ptr);
				break;
			
			default:
				break;
			}
		}
	}

	return true;
}

void Interpreter::Disassemble()
{
	size_t	off = 0;
	size_t	bytesize = program.Size();

	if (bytesize == 0)
		return;

	char*	ptr;
	char*	bytecode = program.Data();

	uint8_t	opcode;
	int64_t	arg1, arg2;
	char	buff[7];
	
	const char* reg[] = {
		"EBP",
		"ESP",
		"EAX",
		"EBX",
		"ECX",
		"EDX",
		"EIP"
	};

	std::cout << "Disassembly:\n";

	while (off < bytesize) {
		ptr = (bytecode + off);

		opcode = *((uint8_t*)ptr);
		arg1 = *((int64_t*)(ptr + sizeof(uint8_t)));
		arg2 = *((int64_t*)(ptr + sizeof(uint8_t) + sizeof(int64_t)));

		sprintf_s(buff, 7, "%04zu: ", off);

		switch (opcode) {
		case OP_PUSH:
			std::cout << buff << "push " << reg[arg1] << "\n";
			break;

		case OP_PUSHADD:
			std::cout << buff << "pushadd " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_POP:
			std::cout << buff << "pop " << reg[arg1] << "\n";
			break;
		
		case OP_MOV_RS:
			std::cout << buff << "mov " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_MOV_RR:
			std::cout << buff << "mov " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_MOV_RM:
			if (arg2 < 0)
				std::cout << buff << "mov " << reg[arg1] << ", [EBP" << arg2 << "]\n";
			else
				std::cout << buff << "mov " << reg[arg1] << ", [EBP+" << arg2 << "]\n";
			break;

		case OP_MOV_MR:
			if (arg1 < 0)
				std::cout << buff << "mov [EBP" << arg1 << "], " << reg[arg2] << "\n";
			else
				std::cout << buff << "mov [EBP+" << arg1 << "], " << reg[arg2] << "\n";
			break;

		case OP_MOV_MM:
			if (arg1 < 0)
				std::cout << buff << "mov [EBP" << arg1;
			else
				std::cout << buff << "mov [EBP+" << arg1;

			if (arg2 < 0)
				std::cout << "], [EBP" << arg2 << "]\n";
			else
				std::cout << "], [EBP+" << arg2 << "]\n";
			break;

		case OP_AND_RS:
			std::cout << buff << "and " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_AND_RR:
			std::cout << buff << "and " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_OR_RS:
			std::cout << buff << "or " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_OR_RR:
			std::cout << buff << "or " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_NOT:
			std::cout << buff << "not " << reg[arg1] << "\n";
			break;

		case OP_ADD_RS:
			std::cout << buff << "add " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_ADD_RR:
			std::cout << buff << "add " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_SUB_RS:
			std::cout << buff << "sub " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_SUB_RR:
			std::cout << buff << "sub " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_MUL_RS:
			std::cout << buff << "mul " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_MUL_RR:
			std::cout << buff << "mul " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_DIV_RS:
			std::cout << buff << "div " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_DIV_RR:
			std::cout << buff << "div " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_MOD_RS:
			std::cout << buff << "mod " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_MOD_RR:
			std::cout << buff << "mod " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_NEG:
			std::cout << buff << "neg " << reg[arg1] << "\n";
			break;

		case OP_SETL_RS:
			std::cout << buff << "setl " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_SETL_RR:
			std::cout << buff << "setl " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_SETLE_RS:
			std::cout << buff << "setle " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_SETLE_RR:
			std::cout << buff << "setle " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_SETG_RS:
			std::cout << buff << "setg " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_SETG_RR:
			std::cout << buff << "setg " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_SETGE_RS:
			std::cout << buff << "setge " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_SETGE_RR:
			std::cout << buff << "setge " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_SETE_RS:
			std::cout << buff << "sete " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_SETE_RR:
			std::cout << buff << "sete " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_SETNE_RS:
			std::cout << buff << "setne " << reg[arg1] << ", " << arg2 << "\n";
			break;

		case OP_SETNE_RR:
			std::cout << buff << "setne " << reg[arg1] << ", " << reg[arg2] << "\n";
			break;

		case OP_JZ:
			std::cout << buff << "jz " << reg[arg1] << ", " << (off + arg2 + ENTRY_SIZE) << "\n";
			break;

		case OP_JNZ:
			std::cout << buff << "jnz " << reg[arg1] << ", " << (off + arg2 + ENTRY_SIZE) << "\n";
			break;

		case OP_JMP:
			std::cout << buff << "jmp " << (off + arg1 + ENTRY_SIZE) << "\n";
			break;

		case OP_PRINT_R:
			std::cout << buff << "print " << reg[arg1] << "\n";
			break;

		case OP_PRINT_M:
			std::cout << buff << "print <addr>\n";
			break;

		default:
			std::cout << "nop\n";
		}

		off += ENTRY_SIZE;
	}
}
