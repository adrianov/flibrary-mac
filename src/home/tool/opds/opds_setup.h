#pragma once

class QCommandLineParser;

namespace Hypodermic
{
class Container;
}

namespace HomeCompa::Opds
{

constexpr auto COLLECTION_NAME = "name";

bool CheckForStop(const QCommandLineParser& parser, Hypodermic::Container& container);
void SetCollection(const QCommandLineParser& parser, Hypodermic::Container& container);

}
