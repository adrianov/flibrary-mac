#pragma once

class QCommandLineParser;

namespace Hypodermic
{
class Container;
}

namespace HomeCompa::Opds
{

constexpr auto APP_ID          = "opds";
constexpr auto COLLECTION_NAME = "name";

int  RunOpds(int argc, char* argv[]);
bool CheckForStop(const QCommandLineParser& parser, Hypodermic::Container& container);
void SetCollection(const QCommandLineParser& parser, Hypodermic::Container& container);

}
