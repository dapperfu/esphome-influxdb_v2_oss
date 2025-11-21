
#include <ctime>

#include "backlog_entry.h"
#include "field.h"
#include "influxdb.h"


namespace esphome {
namespace influxdb {


BacklogEntry::BacklogEntry(const std::string& url, const Field* field, std::string&& value, time_t timestamp):
  url(url),
  field(field),
  value(value),
  timestamp(timestamp)
{
  length = this->field->get_measurement().size() + this->field->get_field_name().size() + this->value.size() + 4 + 10 /*timestamp length*/;
  for (auto tag : this->field->get_tags() ) {
    length += tag.first.size() + tag.second.size() + 2;
  }
}

void BacklogEntry::append( std::string& lines ) {
  size_t l0 = lines.size();
  lines += this->field->get_measurement();
  for (auto tag : this->field->get_tags() ) {
    lines += ',';
    lines += tag.first;
    lines += '=';
    lines += tag.second;
  }
  lines += ' ';
  lines += this->field->get_field_name();
  lines += '=';
  lines += this->value;
  lines += ' ';
  lines += std::to_string( timestamp );
  lines += '\n';
  ESP_LOGV(TAG, "Serialized entry to: %s", lines.c_str()+l0);
}

}  // namespace influxdb
}  // namespace esphome
