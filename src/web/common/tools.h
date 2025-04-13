#pragma once

#include "command_code.h"
#include <optional>
#include <string>

struct Message
{
    OutCommandCode code;
    std::optional<ErrorCode> errorCode;
    std::string message;
};

Message getInMessage(const std::string& data)
{
    size_t pos = data.find(' ');
    auto code = static_cast<OutCommandCode>(std::stoi(data.substr(0, pos)));
    auto message = pos == std::string::npos ? "" : data.substr(pos + 1);

    if (code != OutCommandCode::ERROR)
        return {.code = code, .message = message};

    pos = message.find(' ');

    return {
        .code = code,
        .errorCode = static_cast<ErrorCode>(std::stoi(message.substr(0, pos))),
        .message = pos == std::string::npos ? "" : message.substr(pos + 1)};
}
