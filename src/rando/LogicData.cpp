#include "rando/LogicData.h"

#include <nlohmann/json.hpp>
#include <fstream>

namespace rando {

using nlohmann::json;

// Parse one AST node from its [op, ...] JSON array encoding.
static Req ParseReq(const json& j) {
    Req r;
    const std::string op = j.at(0).get<std::string>();
    if (op == "const") {
        r.op = ReqOp::Const; r.cval = j.at(1).get<bool>();
    } else if (op == "and" || op == "or") {
        r.op = (op == "and") ? ReqOp::And : ReqOp::Or;
        for (const auto& k : j.at(1)) r.kids.push_back(ParseReq(k));
    } else if (op == "not") {
        r.op = ReqOp::Not; r.kids.push_back(ParseReq(j.at(1)));
    } else if (op == "has") {
        r.op = ReqOp::Has; r.a = j.at(1).get<int>(); r.b = j.at(2).get<int>();
    } else if (op == "event") {
        r.op = ReqOp::Event; r.a = j.at(1).get<int>();
    } else if (op == "age") {
        r.op = ReqOp::Age; r.a = j.at(1).get<int>();
    } else if (op == "time") {
        r.op = ReqOp::Time; r.a = j.at(1).get<int>();
    } else if (op == "keys") {
        r.op = ReqOp::Keys; r.a = j.at(1).get<int>(); r.b = j.at(2).get<int>();
    } else if (op == "hearts") {
        r.op = ReqOp::Hearts; r.a = j.at(1).get<int>();
    } else if (op == "tokens") {
        r.op = ReqOp::Tokens; r.a = j.at(1).get<int>();
    } else if (op == "atom") {
        r.op = ReqOp::Atom; r.a = j.at(1).get<int>();
    } else {
        r.op = ReqOp::Const; r.cval = true; // unknown -> permissive (never blocks fill)
    }
    return r;
}

static void ParseGuarded(const json& arr, std::vector<GuardedRef>& out) {
    out.reserve(arr.size());
    for (const auto& e : arr) {
        GuardedRef g;
        g.target = e.at(0).get<int>();
        g.rule = ParseReq(e.at(1));
        out.push_back(std::move(g));
    }
}

bool LogicData::Load(const std::string& path, std::string& error) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { error = "could not open logic asset: " + path; return false; }

    json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        error = std::string("logic asset parse error: ") + e.what();
        return false;
    }

    try {
        version = j.value("version", 0);

        for (const auto& it : j.at("items")) {
            ItemDef d;
            d.key = it.at("key").get<std::string>();
            d.name = it.at("name").get<std::string>();
            d.type = it.value("type", "");
            d.category = it.value("category", "");
            d.progression = it.value("progression", false);
            items.push_back(std::move(d));
        }
        for (const auto& e : j.at("events")) events.push_back(e.get<std::string>());
        for (const auto& l : j.at("locations")) {
            LocationDef d;
            d.key = l.at("key").get<std::string>();
            d.name = l.at("name").get<std::string>();
            d.type = l.value("type", "");
            d.vanilla = l.value("vanilla", "");
            d.shuffled = l.value("shuffled", false);
            locations.push_back(std::move(d));
        }
        for (const auto& d : j.at("dungeons")) dungeons.push_back(d.get<std::string>());
        for (const auto& a : j.at("atoms")) atoms.push_back(a.get<std::string>());

        for (const auto& r : j.at("regions")) {
            Region reg;
            reg.key = r.at("key").get<std::string>();
            reg.name = r.value("name", reg.key);
            reg.scene = r.value("scene", "");
            ParseGuarded(r.at("events"), reg.events);
            ParseGuarded(r.at("checks"), reg.checks);
            ParseGuarded(r.at("exits"), reg.exits);
            regions.push_back(std::move(reg));
        }
    } catch (const std::exception& e) {
        error = std::string("logic asset schema error: ") + e.what();
        return false;
    }

    for (int i = 0; i < (int)items.size(); ++i) itemIndex[items[i].key] = i;
    for (int i = 0; i < (int)events.size(); ++i) eventIndex[events[i]] = i;
    for (int i = 0; i < (int)locations.size(); ++i) locationIndex[locations[i].key] = i;
    for (int i = 0; i < (int)regions.size(); ++i) regionIndex[regions[i].key] = i;

    auto sr = j.value("startRegion", std::string("RR_ROOT"));
    startRegion = RegionIdx(sr);
    if (startRegion < 0) startRegion = 0;
    return true;
}

} // namespace rando
