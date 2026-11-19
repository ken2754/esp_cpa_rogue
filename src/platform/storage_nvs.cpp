#include "storage_nvs.hpp"
#include <Preferences.h>

namespace platform {

static constexpr const char* kNamespace = "rogue_adv";
static constexpr const char* kKeyRecords = "records";

bool loadRecords(rogue::RecordsData& out) {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) {
        rogue::initRecords(out);
        return false;
    }

    size_t len = prefs.getBytesLength(kKeyRecords);
    if (len != sizeof(rogue::RecordsData)) {
        prefs.end();
        rogue::initRecords(out);
        return false;
    }

    rogue::RecordsData tmp;
    size_t readLen = prefs.getBytes(kKeyRecords, &tmp, sizeof(tmp));
    prefs.end();

    if (readLen == sizeof(tmp) && rogue::isValidRecords(tmp)) {
        out = tmp;
        return true;
    }

    rogue::initRecords(out);
    return false;
}

bool saveRecords(const rogue::RecordsData& rec) {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) {
        return false;
    }
    size_t written = prefs.putBytes(kKeyRecords, &rec, sizeof(rec));
    prefs.end();
    return written == sizeof(rec);
}

void clearRecords(rogue::RecordsData& rec) {
    rogue::initRecords(rec);
    saveRecords(rec);
}

} // namespace platform
