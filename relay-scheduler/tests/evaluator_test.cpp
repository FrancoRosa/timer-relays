// Host-side test for the pure Evaluator logic. Builds with plain g++, no
// Arduino toolchain needed:
//
//   g++ -std=c++17 -I ../src -I <ArduinoJson>/src \
//       evaluator_test.cpp ../src/evaluator.cpp -o evaluator_test
//   ./evaluator_test cases.json
//
// Reads tests/cases.json so the same cases can be checked against the
// browser's weekly-chart logic (see the plan's "Weekly chart" section).
#include <ArduinoJson.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../src/evaluator.h"

static std::string readFile(const char* path) {
  std::ifstream f(path, std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

int main(int argc, char** argv) {
  const char* path = argc > 1 ? argv[1] : "cases.json";
  std::string text = readFile(path);
  if (text.empty()) {
    fprintf(stderr, "Could not read %s\n", path);
    return 1;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, text);
  if (err) {
    fprintf(stderr, "JSON parse error in %s: %s\n", path, err.c_str());
    return 1;
  }

  int total = 0;
  int failed = 0;

  for (JsonObject caseObj : doc["cases"].as<JsonArray>()) {
    const char* caseName = caseObj["name"] | "(unnamed case)";

    std::vector<Schedule> schedules;
    for (JsonObject sObj : caseObj["schedules"].as<JsonArray>()) {
      Schedule s;
      s.id = sObj["id"] | -1;
      const char* name = sObj["name"] | "";
      strncpy(s.name, name, MAX_NAME_LEN);
      s.name[MAX_NAME_LEN] = '\0';
      s.ch = sObj["ch"] | 0;
      s.days = sObj["days"] | 0;
      s.start = sObj["start"] | 0;
      s.dur = sObj["dur"] | 0;
      s.en = sObj["en"] | true;
      schedules.push_back(s);
    }

    for (JsonObject sample : caseObj["samples"].as<JsonArray>()) {
      uint32_t weekSecond = sample["weekSecond"];
      uint8_t ch = sample["ch"];
      bool expected = sample["expected"];
      const char* note = sample["note"] | "";

      bool actual = isOn(schedules.data(), schedules.size(), ch, weekSecond);
      total++;
      if (actual != expected) {
        failed++;
        fprintf(stderr, "FAIL [%s] %s: weekSecond=%u ch=%u expected=%s actual=%s\n", caseName,
                note, (unsigned)weekSecond, (unsigned)ch, expected ? "true" : "false",
                actual ? "true" : "false");
      }
    }
  }

  printf("%d/%d checks passed\n", total - failed, total);
  return failed == 0 ? 0 : 1;
}
