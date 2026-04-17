#pragma once

#include <memory>

class IWorldTransferSerializer;

std::unique_ptr<IWorldTransferSerializer> CreatePlayerCharacterTransferSerializer();
