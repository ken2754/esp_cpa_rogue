#pragma once

#include "../game/records.hpp"

namespace platform {

bool loadRecords(rogue::RecordsData& out);
bool saveRecords(const rogue::RecordsData& rec);
void clearRecords(rogue::RecordsData& rec);

} // namespace platform
