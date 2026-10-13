#ifndef BUBBLEFIGHTER_SERVER_FEATURES_CONFIGURATION_CONTENT_CONFIG_RESPONSE_H_
#define BUBBLEFIGHTER_SERVER_FEATURES_CONFIGURATION_CONTENT_CONFIG_RESPONSE_H_

#include "protocol/configuration/dynamic_config_request.h"
#include "protocol/configuration/dynamic_config_response.h"

namespace bubble_fighter::server::features::configuration {
protocol::DynamicConfigResponse RespondContentConfiguration(
    const protocol::DynamicConfigRequest& request);
}

#endif  // BUBBLEFIGHTER_SERVER_FEATURES_CONFIGURATION_CONTENT_CONFIG_RESPONSE_H_
