#include "absolute_precision_type.h"
#include "log.hpp"

int main()
{
    absolute_precision_type num1, num2, num3;
    num1 = absolute_precision_type::make_absolute_precision_type("1");
    num2 = absolute_precision_type::make_absolute_precision_type("0.288289013214");
    num3 = absolute_precision_type::make_absolute_precision_type("0.112233344553321");

    std::cout << num1.to_string() << std::endl;
    std::cout << num2.to_string() << std::endl;
    std::cout << (num2 * num3).to_string() << std::endl;
}
