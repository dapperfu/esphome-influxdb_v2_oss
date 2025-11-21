
#include "text_sensor_field.h"

namespace esphome {
namespace influxdb {

#ifdef USE_TEXT_SENSOR

void TextSensorField::do_setup() {}

std::string TextSensorField::to_value() const {
  const std::string& state = this->sensor_->get_state();
  std::string value;
  value.reserve(state.size() + 2);  // Two quotes + state string
  value += '"';
  value += state;
  value += '"';
  return value;
}

#endif

}  // namespace influxdb
}  // namespace esphome
