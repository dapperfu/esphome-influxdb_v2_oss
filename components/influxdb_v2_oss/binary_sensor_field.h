#pragma once

#include "field.h"

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace influxdb {

class BinarySensorField : public Field {
public:
  void set_sensor(binary_sensor::BinarySensor *sensor) { this->sensor_ = sensor; }

  void do_setup() override;
  bool sensor_has_state() const override { return this->sensor_->has_state(); }
  std::string sensor_object_id() const override { return this->sensor_->get_object_id(); }
  std::string sensor_object_name() const override { return this->sensor_->get_name(); }
  std::string sensor_object_device_class() const override { return this->sensor_->get_device_class(); }
  std::string to_value() const override;

protected:
  binary_sensor::BinarySensor *sensor_;
};

}  // namespace influxdb
}  // namespace esphome

#endif
