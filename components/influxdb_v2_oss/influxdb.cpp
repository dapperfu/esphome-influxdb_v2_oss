#include "influxdb.h"

#include "backlog_entry.h"
#include "binary_sensor_field.h"
#include "numeric_sensor_field.h"
#include "text_sensor_field.h"

namespace esphome {
namespace influxdb {

// J2000 epoch: 2000-01-01 00:00:00 UTC (946684800 seconds since Unix epoch)
// Using 946681200 as threshold to check if we have an absolute time
static constexpr time_t MIN_VALID_TIMESTAMP = 946681200;

void InfluxDB::setup() {
  http_request::Header header;

  header.name = "Content-Type";
  header.value = "text/plain; charset=utf-8";
  this->headers_.push_back(header);

  header.name = "Content-Encoding";
  header.value = "identity";
  this->headers_.push_back(header);

  header.name = "Accept";
  header.value = "application/json";
  this->headers_.push_back(header);

  if (!this->token_.empty()) {
    header.name = "Authorization";
    header.value = this->token_.c_str();
    this->headers_.push_back(header);
  }

#ifdef USE_BINARY_SENSOR
  if (this->publish_all_ || this->publish_all_binary_) {
    for (auto *obj : App.get_binary_sensors()) {
      App.feed_wdt();
      if (!obj->is_internal() && std::none_of(this->fields_.begin(), this->fields_.end(), [&obj](Field* o) { return o->sensor_object_id() == obj->get_object_id(); })) {
        BinarySensorField* field = new BinarySensorField();
        field->set_sensor(obj);
        field->set_measurement( this->measurement_ );
        obj->add_on_state_callback([this, field](bool state) {
          this->queue( std::move(field->to_entry( this->url_ )) );
        });
        this->add_field( field );
      }
    }
  }
#endif
#ifdef USE_SENSOR
  if (this->publish_all_ || this->publish_all_numeric_) {
    for (auto *obj : App.get_sensors()) {
      App.feed_wdt();
      if (!obj->is_internal() && std::none_of(this->fields_.begin(), this->fields_.end(), [&obj](Field* o) { return o->sensor_object_id() == obj->get_object_id(); })) {
        NumericSensorField* field = new NumericSensorField();
        field->set_sensor(obj);
        field->set_measurement( this->measurement_ );
        obj->add_on_state_callback([this, field](float state) {
          this->queue( std::move(field->to_entry( this->url_ )) );
        });
        this->add_field( field );
      }
    }
  }
#endif
#ifdef USE_TEXT_SENSOR
  if (this->publish_all_ || this->publish_all_text_) {
    for (auto *obj : App.get_text_sensors()) {
      App.feed_wdt();
      if (!obj->is_internal() && std::none_of(this->fields_.begin(), this->fields_.end(), [&obj](Field* o) { return o->sensor_object_id() == obj->get_object_id(); })) {
        TextSensorField* field = new TextSensorField();
        field->set_sensor(obj);
        field->set_measurement( this->measurement_ );
        obj->add_on_state_callback([this, field](std::string state) {
          this->queue( std::move(field->to_entry( this->url_ )) );
        });
        this->add_field( field );
      }
    }
  }
#endif

  for (auto field : this->fields_) {
    App.feed_wdt();
    field->set_clock( this->clock_ );
    for (auto tag : this->global_tags_)
      field->add_tag( tag.first, tag.second );
    field->setup( this->default_name_policy );
  }
}

void InfluxDB::dump_config() {
  ESP_LOGCONFIG(TAG, "InfluxDB component");
  ESP_LOGCONFIG(TAG, "  Fields:");
  for (auto field : this->fields_)
    ESP_LOGCONFIG(TAG, "    %s", field->get_field_name().c_str());
}

void InfluxDB::loop() {
  if (!this->backlog_.empty()) {
    ESP_LOGD(TAG, "Sending InfluxDB lines to server (queue size: %d)", this->backlog_.size());
    uint8_t item_count = 0;
    do {
      auto& m = this->backlog_.front();

      // Find all queued messages that go to the same url
      std::vector<size_t> active_indices;
      size_t len = 0;
      for (size_t i = 0; i < this->backlog_.size(); ++i) {
        if (this->backlog_[i].url == m.url && active_indices.size() < this->backlog_drain_batch_) {
          active_indices.push_back(i);
          len += this->backlog_[i].length;
        }
      }

      std::string body;
      ESP_LOGD(TAG, "Reserving memory for influxdb POST body: %d b", len);
      body.reserve(len);
      for (auto idx : active_indices)
        this->backlog_[idx].append( body );
      bool success = this->send_data(m.url, std::move(body));

      if (success) {
        // Erase in reverse order to maintain valid indices
        for (auto it = active_indices.rbegin(); it != active_indices.rend(); ++it) {
          item_count++;
          this->backlog_.erase(this->backlog_.begin() + *it);
        }
      } else {
        break;
      }
    } while (!this->backlog_.empty() && (item_count < this->backlog_drain_batch_));
    ESP_LOGD(TAG, "Drained %d items from backlog", item_count);
  } else {
    this->disable_loop();
  }
}

void InfluxDB::queue(BacklogEntry&& data) {
  if (data.timestamp < MIN_VALID_TIMESTAMP) {
    ESP_LOGW(TAG, "Cannot submit influxdb metrics for %s, clock is not ready!", data.field->get_field_name().c_str());
    return;
  }

  ESP_LOGD(TAG, "Adding data (%d) into the InfluxDB queue for %s", data.length, data.url.c_str());
  if (this->backlog_.size() == this->backlog_max_depth_) {
    ESP_LOGW(TAG, "Backlog is full, dropping oldest entries.");
    this->backlog_.erase(this->backlog_.begin());
  }
  this->backlog_.push_back(data);
  this->enable_loop();
}

bool InfluxDB::send_data(const std::string &url, const std::string &data) {
  uint8_t buf[32];
  auto response = this->http_request_->post(url, data, this->headers_);

  if (response == nullptr) {
    ESP_LOGW(TAG, "Error sending metrics to %s", url.c_str());
    return false;
  }
  auto status_code = response->status_code;
  if (status_code != 204) {
    ESP_LOGW(TAG, "Error sending metrics to %s: HTTP %d", url.c_str(), status_code);
    return false;
  }

  // Drain the response
  while (response->read(buf, sizeof(buf)) != 0) {}
  response->end();

  return true;
}

}  // namespace influxdb
}  // namespace esphome
