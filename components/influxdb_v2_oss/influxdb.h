#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/components/http_request/http_request.h"
#include "esphome/components/time/real_time_clock.h"

#include <list>
#include <vector>

namespace esphome {
namespace influxdb {

static const char *const TAG = "influxdb_v2_oss";

class Field;
class BacklogEntry;

enum DefaultNamePolicy {
  ENTITY_NAME,
  ENTITY_ID,
  ENTITY_DEVICE_CLASS
};

class InfluxDB : public Component {
public:
  void setup() override;
  void dump_config() override;
  void loop() override;
  void set_http_request(http_request::HttpRequestComponent *http) { this->http_request_ = http; };
  void set_url(std::string url) { this->url_ = std::move(url); }
  void set_token(std::string token) { this->token_ = std::string("Token ") + token; }
  void set_measurement(std::string measurement) { this->measurement_ = measurement; }
  void set_default_name_policy(DefaultNamePolicy default_name_policy) { this->default_name_policy = default_name_policy; }
  void set_publish_all(bool publish_all) { this->publish_all_ = publish_all; }
  void set_publish_all_numeric(bool publish_all_numeric) { this->publish_all_numeric_ = publish_all_numeric; }
  void set_publish_all_binary(bool publish_all_binary) { this->publish_all_binary_ = publish_all_binary; }
  void set_publish_all_text(bool publish_all_text) { this->publish_all_text_ = publish_all_text; }

  void add_tag( const std::string key, const std::string value ) { this->global_tags_.push_back(std::pair(key, value)); }
  void set_clock(time::RealTimeClock *clock) { this->clock_ = clock; }
  void set_backlog_max_depth(uint8_t val) { this->backlog_max_depth_ = val; }
  void set_backlog_drain_batch(uint8_t val) { this->backlog_drain_batch_ = val; }
  void add_field(Field* field) { this->fields_.push_back(field); }
  const std::string &get_url() { return this->url_; }

  void queue(BacklogEntry&& entry);

  float get_setup_priority() const override { return setup_priority::LATE; }

protected:
  bool send_data(const std::string &url, const std::string &data);

  bool publish_all_;
  bool publish_all_numeric_;
  bool publish_all_binary_;
  bool publish_all_text_;
  DefaultNamePolicy default_name_policy;
  std::list<std::pair<const std::string, const std::string>> global_tags_;
  std::string measurement_;

  http_request::HttpRequestComponent *http_request_;
  std::string url_;
  std::string token_;
  std::list<http_request::Header> headers_;
  std::list<Field*> fields_;

  time::RealTimeClock *clock_{nullptr};
  std::vector<BacklogEntry> backlog_;
  uint8_t backlog_max_depth_{20};
  uint8_t backlog_drain_batch_{5};
};

}  // namespace influxdb
}  // namespace esphome
