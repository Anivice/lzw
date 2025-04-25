#include "absolute_precision_type.h"
#include "log.hpp"

int main()
{
    absolute_precision_type num1 = absolute_precision_type::make_absolute_precision_type("1");
    absolute_precision_type num2 = absolute_precision_type::make_absolute_precision_type("0.288289013214");
    absolute_precision_type num3 = absolute_precision_type::make_absolute_precision_type("0.112233344553321");
    absolute_precision_type num4 = absolute_precision_type::make_absolute_precision_type("0.74213211311111111111111111111");
    auto num5 = (num4 - num2 * num3);

    std::cout << num1.to_string() << std::endl;
    std::cout << num2.to_string() << std::endl;
    std::cout << num3.to_string() << std::endl;
    std::cout << num4.to_string() << std::endl;
    std::cout << num5.to_string() << std::endl;
    std::cout << (num1 / 3).to_string() << std::endl;

    debug::log(debug::warning_log, num4 > num1, "\n");
}
