#include <rclcpp/rclcpp.hpp>

#include "ExternalReferee.hpp"

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto ref = std::make_shared<referee::ExternalReferee>();
    rclcpp::spin(ref);
}