
#include <string>

#include "field.h"
#include "influxdb.h"
#include "backlog_entry.h"

namespace esphome {
namespace influxdb {

static std::string escape_influxdb_string(const std::string& input) {
  std::string result;
  result.reserve(input.size() + input.size() / 4);  // Estimate: ~25% overhead for escaped chars
  for (char c : input) {
    if (c == ',' || c == ' ' || c == '=' || c == '\\') {
      result += '\\';
    }
    result += c;
  }
  return result;
}

void Field::setup(DefaultNamePolicy default_name_policy) {
  auto device_class = this->sensor_object_device_class();
  if (this->get_field_name().empty()) {
    switch (default_name_policy) {
    case DefaultNamePolicy::ENTITY_NAME:
      this->set_field_name( str_sanitize(this->sensor_object_name()) );
      break;
    case DefaultNamePolicy::ENTITY_DEVICE_CLASS:
      if (!device_class.empty()) {
        this->set_field_name( this->sensor_object_device_class() );
      } else {
        ESP_LOGW(TAG, "Sensor with id=%s does not have a device_class set! Falling back to using sensor id as field name!", this->sensor_object_id().c_str());
        this->set_field_name( str_sanitize(this->sensor_object_name()) );
      }
      break;
    case DefaultNamePolicy::ENTITY_ID:
    default:
      this->set_field_name( this->sensor_object_id() );
      break;
    }
  }
  this->add_tag(STR_TAG_SENSOR_ID, this->sensor_object_id());
  this->add_tag(STR_TAG_SENSOR_NAME,  this->sensor_object_name());
  if (!device_class.empty())
    this->add_tag(STR_TAG_DEVICE_CLASS, device_class);
  this->do_setup();
}

void Field::add_tag(const std::string& tag, const std::string &value) {
  std::string escaped_tag = escape_influxdb_string(tag);
  std::string escaped_value = escape_influxdb_string(value);
  this->tags_.emplace_back( escaped_tag, escaped_value );
}

BacklogEntry Field::to_entry( const std::string& url ) {
  time_t now = this->clock_->timestamp_now();
  std::string value = this->to_value();
  return BacklogEntry(url, this, std::move(value), now);
}

}  // namespace influxdb
}  // namespace esphome
