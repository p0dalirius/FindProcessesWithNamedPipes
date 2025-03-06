#ifndef ARGUMENTPARSER_H
#define ARGUMENTPARSER_H

#include <iostream>
#include <list>
#include <map>
#include <string>
#include <variant>
#include "Argument.h"
#include "ArgumentType.h"


using type_of_arguments_value = std::variant<bool, int, std::string>;


class ArgumentsParser
{
    public:
        /* Add arguments */
        void add_positional_string_argument(const std::string& name, const std::string& help);
        void add_boolean_switch_argument(const std::string& name, const std::string& shortoption, const std::string& longoption, bool defaultValue, bool required, const std::string& help);
        void add_string_argument(const std::string& name, const std::string& shortoption, const std::string& longoption, const std::string& defaultValue, bool required, const std::string& help);
        void add_int_argument(const std::string& name, const std::string& shortoption, const std::string& longoption, const std::string& defaultValue, bool required, const std::string& help);
        
        /* Core methods */
        int parse_args(int argc, char* argv[]);
        type_of_arguments_value get_value(const char* argument_name);
        void add_subparser(std::string name);
        void help(int argc, char* argv[]);
        
        void debug();

        ArgumentsParser();

    private:
        std::map<std::string, type_of_arguments_value> arguments;

        std::list<Argument> positionalArguments;
        std::list<Argument> allArguments;
        std::list<Argument> mandatoryArguments;
        std::list<Argument> optionalArguments;
};

#endif

