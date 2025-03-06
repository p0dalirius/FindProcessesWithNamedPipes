#include "ArgumentsParser.h"


ArgumentsParser::ArgumentsParser() {
	this->add_boolean_switch_argument("help", "-h", "--help", false, false, "Displays this help message.");
}


int ArgumentsParser::parse_args(int argc, char* argv[]) {
	int current_arg_index = 1;
	bool show_help = false;
	bool mandatory_argument_found = false;

	for (auto& arg_ptr : this->optionalArguments) {
		if (arg_ptr.name == "help") {
			for (int k = 1; k < argc; k++) {
				if ((argv[k] == arg_ptr.shortoption) || (argv[k] == arg_ptr.longoption)) {
					show_help = true;
				}
			}
		}
	}

	if (show_help == false) {
		if ((unsigned int)argc < this->positionalArguments.size()) {
			show_help = true;
		}
		else {
			for (auto& arg_ptr : this->mandatoryArguments) {
				mandatory_argument_found = false;
				for (int k = 1; k < argc; k++) {
					if ((argv[k] == arg_ptr.shortoption) || (argv[k] == arg_ptr.longoption)) {
						mandatory_argument_found = true;
					}
				}
				if (mandatory_argument_found == false) {
					show_help = true;
				}
			}
		}
	}

	if (show_help == true) {
		this->help(argc, argv);
		return -1;
	} else {
		// Parse positional arguments from the command line 
		for (auto& arg_ptr : this->positionalArguments) {
			current_arg_index = arg_ptr.parse(argc, argv, current_arg_index);
			this->arguments.insert({ arg_ptr.name, arg_ptr.value });
		}
		// Parse all other arguments from the command line
		for (auto& arg_ptr : this->allArguments) {
			current_arg_index = arg_ptr.parse(argc, argv, current_arg_index);
			this->arguments.insert({ arg_ptr.name, arg_ptr.value });
		}
		return 0;
	}
}


/* Add arguments */
void ArgumentsParser::add_positional_string_argument(const std::string& name, const std::string& help) {
	Argument arg = Argument(
		name, 
		ArgumentType::PositionalStringArgument, 
		static_cast<std::string>(""),
		static_cast<std::string>(""),
		static_cast<std::string>(""),
		true, 
		help
	);
	this->positionalArguments.push_back(arg);
}


void ArgumentsParser::add_boolean_switch_argument(const std::string& name, const std::string& shortoption, const std::string& longoption, bool defaultValue, bool required, const std::string& help) {
	Argument arg;
	arg = Argument(
		name,
		ArgumentType::BooleanSwitchArgument,
		shortoption,
		longoption,
		defaultValue,
		required,
		help
	);

	if (arg.required == true) {
		this->mandatoryArguments.push_back(arg);
	} else {
		this->optionalArguments.push_back(arg);
	}
	this->allArguments.push_back(arg);
}


void ArgumentsParser::add_string_argument(const std::string& name, const std::string& shortoption, const std::string& longoption, const std::string& defaultValue, bool required, const std::string& help) {
	Argument arg = Argument(
		name,
		ArgumentType::StringArgument,
		shortoption,
		longoption,
		defaultValue,
		required,
		help
	);
	if (arg.required == true) {
		this->mandatoryArguments.push_back(arg);
	}
	else {
		this->optionalArguments.push_back(arg);
	}
	this->allArguments.push_back(arg);
}


void ArgumentsParser::add_int_argument(const std::string& name, const std::string& shortoption, const std::string& longoption, const std::string& defaultValue, bool required, const std::string& help) {
	Argument arg = Argument(
		name,
		ArgumentType::IntegerArgument,
		shortoption,
		longoption,
		defaultValue,
		required,
		help
	);
	if (arg.required == true) {
		this->mandatoryArguments.push_back(arg);
	}
	this->allArguments.push_back(arg);
}


type_of_arguments_value ArgumentsParser::get_value(const char* argument_name) {
	return this->arguments[argument_name];
}


void ArgumentsParser::help(int argc, char * argv[]) {
	std::string programpath = ((std::string)argv[0]);
	std::string programname = programpath.substr(programpath.find_last_of("/\\") + 1);

	type_of_arguments_value value;
	std::string name;

	std::cout << "Usage: " << programname << " ";

	for (auto& arg_ptr : this->positionalArguments) {
		std::cout << "<" << arg_ptr.name << "> ";
	}

	this->allArguments.sort();
	for (auto& arg_ptr : this->allArguments) {
		std::cout << "[" << arg_ptr.shortoption << " " << arg_ptr.name << "] ";
	}
	std::cout << "\n\n";

	if (this->positionalArguments.size() != 0) {
		std::cout << "Positional arguments:\n";
		for (auto& arg_ptr : this->positionalArguments) {
			std::cout << "   <" << arg_ptr.name << ">   " << arg_ptr.help << "\n";
		}
		std::cout << "\n";
	}

	if (this->mandatoryArguments.size() != 0) {
		std::cout << "Required arguments:\n";
		for (auto& arg_ptr : this->mandatoryArguments) {
			std::cout << "   " << arg_ptr.shortoption << ", " << arg_ptr.longoption << " " << arg_ptr.help << "\n";
		}
		std::cout << "\n";
	}

	if (this->optionalArguments.size() != 0) {
		std::cout << "Optional arguments:\n";
		for (auto& arg : this->optionalArguments) {
			if (std::holds_alternative<std::string>(arg.value)) {
				std::cout << "   " << arg.shortoption << ", " << arg.longoption << " " << arg.help << " (default: " << std::get<std::string>(arg.value) << ")" << "\n";
			}
			else if (std::holds_alternative<int>(arg.value)) {
				std::cout << "   " << arg.shortoption << ", " << arg.longoption << " " << arg.help << " (default: " << std::get<int>(arg.value) << ")" << "\n";
			}
			else if (std::holds_alternative<bool>(arg.value)) {
				if (std::get<bool>(arg.value) == true) {
					std::cout << "   " << arg.shortoption << ", " << arg.longoption << " " << arg.help << " (default: " << "true" << ")" << "\n";
				}
				else {
					std::cout << "   " << arg.shortoption << ", " << arg.longoption << " " << arg.help << " (default: " << "false" << ")" << "\n";
				}
			}
		}
		std::cout << "\n";
	}
	// Exits the program
	exit(0);
}


void ArgumentsParser::debug() {
	type_of_arguments_value value;
	std::string name;

	std::cout << "[debug] len(parser.arguments) = " << this->arguments.size() << "\n";

	for (auto& arg: this->arguments) {
		name = std::get<0>(arg);
		value = std::get<1>(arg);
		if (std::holds_alternative<std::string>(value)) {
			std::cout << " - (string) " << name << " = " << std::get<std::string>(value) << "\n";
		} else if (std::holds_alternative<int>(value)) {
			std::cout << " - (int)    " << name << " = " << std::get<int>(value) << "\n";
		} else if (std::holds_alternative<bool>(value)) {
			if (std::get<bool>(value) == true) {
				std::cout << " - (bool)   " << name << " = " << "true" << "\n";
			}
			else {
				std::cout << " - (bool)   " << name << " = " << "false" << "\n";
			}
		}
	}
}