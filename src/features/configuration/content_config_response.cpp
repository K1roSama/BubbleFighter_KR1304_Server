#include "features/configuration/content_config_response.h"

#include "protocol/codec/zlib_stored_stream.h"
#include "protocol/configuration/content_configuration.h"

namespace bubble_fighter::server::features::configuration {
protocol::DynamicConfigResponse RespondContentConfiguration(
    const protocol::DynamicConfigRequest& request) {
  using namespace protocol;
  // Local policy for this client/resource set, not recovered production
  // settings: four scene characters, no preview overrides, no extra time
  // offset, and a positive local gauge maximum of 100. See E037.
  static const auto kCompressed = ZlibStoredStream::Encode(ContentConfiguration{
      u"Ui/DlgCreateUser3.ui", {10, 19, 25, 27}, {}, 0, 100}
                                                               .Encode());
  static const auto kChecksum = ZlibStoredStream::Adler32(kCompressed, 0);
  DynamicConfigResponse response;
  const auto& cached = request.checksums[ContentConfiguration::kCategory];
  if (cached && *cached != kChecksum) {
    response.records[ContentConfiguration::kCategory] = kCompressed;
  }
  return response;
}
}  // namespace bubble_fighter::server::features::configuration
