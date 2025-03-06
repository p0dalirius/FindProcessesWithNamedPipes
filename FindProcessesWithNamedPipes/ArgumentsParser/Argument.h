#ifndef ARGUMENT_H
#define ARGUMENT_H

#include <string>
#include <variant>
#include "ArgumentType.h"

using type_of_arguments_value = std::variant<bool, int, std::string>;


class Argument
{
    public:
        std::string name;
        ArgumentType argumentType;
        std::string shortoption;
        std::string longoption;
        type_of_arguments_value value;
        type_of_arguments_value defaultValue;
        bool required;
        std::string help;
    
    Argument();
    
    Argument(const std::string& name, ArgumentType argumentType, const std::string& shortoption, const std::string& longoption, const type_of_arguments_value& defaultValue, bool required, const std::string& help);
    
    virtual int parse(int argc, char* argv[], int current_index);

    /* Comparison operators */

    bool operator==(const Argument& arg) const {
        return (this->name == arg.name);
    }

    bool operator<(const Argument& arg) const {
        return (this->name < arg.name);
    }

    bool operator>(const Argument& arg) const {
        return (this->name > arg.name);
    }

    bool operator>=(const Argument& arg) const {
        return (this->name >= arg.name);
    }

    bool operator<=(const Argument& arg) const {
        return (this->name <= arg.name);
    }

    bool operator!=(const Argument& arg) const {
        return (this->name != arg.name);
    }
};

#endif
