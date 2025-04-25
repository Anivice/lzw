#include "absolute_precision_type.h"
#include "log.hpp"

int main()
{
    absolute_precision_type num1 = absolute_precision_type::make_absolute_precision_type("1");
    absolute_precision_type num2 = absolute_precision_type::make_absolute_precision_type("0.9");
    absolute_precision_type num3 = absolute_precision_type::make_absolute_precision_type("0.1");

    std::cout << num1.to_string() << std::endl;
    std::cout << num2.to_string() << std::endl;
    std::cout << num3.to_string() << std::endl;
    std::cout << (num2 * num3).to_string() << std::endl;
}
