#pragma once

#include <memory>

#include "interface/logic/IJokeRequester.h"

namespace HomeCompa::Flibrary
{

std::shared_ptr<IJokeRequester::IClient> CreateJokeRequesterClient(IJokeRequester::IClient& impl);

}
