#include "Engine.h"

void Engine::process(ParsedInductiveType p)
{
    identifiers[std::string{ p.identifier.identifier }] = p;
}