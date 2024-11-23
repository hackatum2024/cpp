#include "crow_all.h"
#include <algorithm>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Data structures
struct Offer {
  std::string id;
  std::string data;
  int32_t mostSpecificRegionID;
  int64_t startDate;
  int64_t endDate;
  uint8_t numberSeats;
  uint16_t price;
  std::string carType;
  bool hasVollkasko;
  uint16_t freeKilometers;
};

struct PriceRange {
  uint16_t start;
  uint16_t end;
  uint32_t count;
};

struct CarTypeCount {
  uint32_t small;
  uint32_t sports;
  uint32_t luxury;
  uint32_t family;
};

struct VollkaskoCount {
  uint32_t trueCount;
  uint32_t falseCount;
};

struct SeatsCount {
  uint8_t numberSeats;
  uint32_t count;
};

struct FreeKilometerRange {
  uint16_t start;
  uint16_t end;
  uint32_t count;
};

// Helper functions for aggregations
std::vector<PriceRange> calculatePriceRanges(const std::vector<Offer> &offers,
                                             uint32_t priceRangeWidth) {
  if (offers.empty() || priceRangeWidth == 0) {
    return {};
  }

  // Find min and max prices
  auto [minIt, maxIt] = std::minmax_element(
      offers.begin(), offers.end(),
      [](const Offer &a, const Offer &b) { return a.price < b.price; });

  uint16_t minPrice = minIt->price;
  uint16_t maxPrice = maxIt->price;

  // Calculate number of ranges needed
  size_t numRanges =
      (maxPrice - minPrice + priceRangeWidth - 1) / priceRangeWidth;
  std::vector<uint32_t> rangeCounts(numRanges, 0);

  // Count offers in each range
  for (const auto &offer : offers) {
    size_t rangeIndex = (offer.price - minPrice) / priceRangeWidth;
    if (rangeIndex < rangeCounts.size()) {
      rangeCounts[rangeIndex]++;
    }
  }

  // Create PriceRange objects for non-empty ranges
  std::vector<PriceRange> result;
  for (size_t i = 0; i < rangeCounts.size(); i++) {
    if (rangeCounts[i] > 0) {
      uint16_t start = minPrice + i * priceRangeWidth;
      uint16_t end = start + priceRangeWidth;
      result.push_back({start, end, rangeCounts[i]});
    }
  }

  return result;
}

CarTypeCount calculateCarTypeCounts(const std::vector<Offer> &offers) {
  CarTypeCount counts = {0, 0, 0, 0};

  for (const auto &offer : offers) {
    if (offer.carType == "small")
      counts.small++;
    else if (offer.carType == "sports")
      counts.sports++;
    else if (offer.carType == "luxury")
      counts.luxury++;
    else if (offer.carType == "family")
      counts.family++;
  }

  return counts;
}

std::vector<SeatsCount> calculateSeatsCount(const std::vector<Offer> &offers) {
  std::map<uint8_t, uint32_t> seatCounts;

  // Count offers for each seat number
  for (const auto &offer : offers) {
    seatCounts[offer.numberSeats]++;
  }

  // Convert to vector of SeatsCount
  std::vector<SeatsCount> result;
  for (const auto &[seats, count] : seatCounts) {
    result.push_back({seats, count});
  }

  return result;
}

std::vector<FreeKilometerRange>
calculateFreeKilometerRanges(const std::vector<Offer> &offers,
                             uint32_t minFreeKilometerWidth) {
  if (offers.empty() || minFreeKilometerWidth == 0) {
    return {};
  }

  // Find min and max free kilometers
  auto [minIt, maxIt] = std::minmax_element(
      offers.begin(), offers.end(), [](const Offer &a, const Offer &b) {
        return a.freeKilometers < b.freeKilometers;
      });

  uint16_t minKm = minIt->freeKilometers;
  uint16_t maxKm = maxIt->freeKilometers;

  // Calculate number of ranges needed
  size_t numRanges =
      (maxKm - minKm + minFreeKilometerWidth - 1) / minFreeKilometerWidth;
  std::vector<uint32_t> rangeCounts(numRanges, 0);

  // Count offers in each range
  for (const auto &offer : offers) {
    size_t rangeIndex = (offer.freeKilometers - minKm) / minFreeKilometerWidth;
    if (rangeIndex < rangeCounts.size()) {
      rangeCounts[rangeIndex]++;
    }
  }

  // Create FreeKilometerRange objects for non-empty ranges
  std::vector<FreeKilometerRange> result;
  for (size_t i = 0; i < rangeCounts.size(); i++) {
    if (rangeCounts[i] > 0) {
      uint16_t start = minKm + i * minFreeKilometerWidth;
      uint16_t end = start + minFreeKilometerWidth;
      result.push_back({start, end, rangeCounts[i]});
    }
  }

  return result;
}

VollkaskoCount calculateVollkaskoCounts(const std::vector<Offer> &offers) {
  VollkaskoCount counts = {0, 0};

  for (const auto &offer : offers) {
    if (offer.hasVollkasko) {
      counts.trueCount++;
    } else {
      counts.falseCount++;
    }
  }

  return counts;
}

// Global storage
std::vector<Offer> offers;
std::mutex offers_mutex;

// Helper functions
bool isValidCarType(const std::string &type) {
  return type == "small" || type == "sports" || type == "luxury" ||
         type == "family";
}

int main() {
  crow::SimpleApp app;

  // POST /api/offers - Create new offers
  CROW_ROUTE(app, "/api/offers")
      .methods(
          "POST"_method)([](const crow::request &req, crow::response &res) {
        auto json = crow::json::load(req.body);
        if (!json) {
          res.code = 400;
          res.write("Invalid JSON");
          res.end();
          return;
        }

        if (!json.has("offers")) {
          res.code = 400;
          res.write("Missing offers array");
          res.end();
          return;
        }

        std::vector<Offer> newOffers;
        for (const auto &offerJson : json["offers"]) {
          Offer offer;
          try {
            offer.id = offerJson["ID"].s();
            offer.data = offerJson["data"].s();
            offer.mostSpecificRegionID = offerJson["mostSpecificRegionID"].i();
            offer.startDate = offerJson["startDate"].i();
            offer.endDate = offerJson["endDate"].i();
            offer.numberSeats = offerJson["numberSeats"].i();
            offer.price = offerJson["price"].i();
            offer.carType = offerJson["carType"].s();
            offer.hasVollkasko = offerJson["hasVollkasko"].b();
            offer.freeKilometers = offerJson["freeKilometers"].i();

            if (!isValidCarType(offer.carType)) {
              res.code = 400;
              res.write("Invalid car type");
              res.end();
              return;
            }

            newOffers.push_back(offer);
          } catch (const std::exception &e) {
            res.code = 400;
            res.write("Invalid offer data");
            res.end();
            return;
          }
        }

        // Add offers to storage
        {
          std::lock_guard<std::mutex> lock(offers_mutex);
          offers.insert(offers.end(), newOffers.begin(), newOffers.end());
        }

        res.code = 200;
        res.end();
      });

  // GET /api/offers - Search offers
  CROW_ROUTE(app, "/api/offers")
      .methods("GET"_method)([](const crow::request &req, crow::response &res) {
        try {
          int32_t regionID = std::stoi(req.url_params.get("regionID"));
          int64_t timeRangeStart =
              std::stoll(req.url_params.get("timeRangeStart"));
          int64_t timeRangeEnd = std::stoll(req.url_params.get("timeRangeEnd"));
          uint16_t numberDays = std::stoi(req.url_params.get("numberDays"));
          std::string sortOrder = req.url_params.get("sortOrder");
          uint32_t page = std::stoul(req.url_params.get("page"));
          uint32_t pageSize = std::stoul(req.url_params.get("pageSize"));
          uint32_t priceRangeWidth =
              std::stoul(req.url_params.get("priceRangeWidth"));
          uint32_t minFreeKilometerWidth =
              std::stoul(req.url_params.get("minFreeKilometerWidth"));

          // Create response object
          crow::json::wvalue response;
          std::vector<crow::json::wvalue> resultOffers;

          // Filter offers based on mandatory parameters
          std::vector<Offer> filteredOffers;
          {
            std::lock_guard<std::mutex> lock(offers_mutex);
            for (const auto &offer : offers) {
              if (offer.mostSpecificRegionID == regionID &&
                  offer.startDate >= timeRangeStart &&
                  offer.endDate <= timeRangeEnd) {
                filteredOffers.push_back(offer);
              }
            }
          }

          // Sort offers
          if (sortOrder == "price-asc") {
            std::sort(filteredOffers.begin(), filteredOffers.end(),
                      [](const Offer &a, const Offer &b) {
                        return a.price < b.price ||
                               (a.price == b.price && a.id < b.id);
                      });
          } else if (sortOrder == "price-desc") {
            std::sort(filteredOffers.begin(), filteredOffers.end(),
                      [](const Offer &a, const Offer &b) {
                        return a.price > b.price ||
                               (a.price == b.price && a.id < b.id);
                      });
          }

          // Paginate results
          size_t startIdx = page * pageSize;
          size_t endIdx = std::min(startIdx + pageSize, filteredOffers.size());

          // Prepare offers for response
          for (size_t i = startIdx; i < endIdx; i++) {
            crow::json::wvalue offerJson;
            offerJson["ID"] = filteredOffers[i].id;
            offerJson["data"] = filteredOffers[i].data;
            resultOffers.push_back(std::move(offerJson));
          }

          response["offers"] = std::move(resultOffers);

          // Add dummy aggregations (to be implemented)
          // response["priceRanges"] = std::vector<crow::json::wvalue>();
          // response["carTypeCounts"] = crow::json::wvalue(
          //     {{"small", 0}, {"sports", 0}, {"luxury", 0}, {"family", 0}});
          // response["seatsCount"] = std::vector<crow::json::wvalue>();
          // response["freeKilometerRange"] = std::vector<crow::json::wvalue>();
          // response["vollkaskoCount"] =
          //     crow::json::wvalue({{"trueCount", 0}, {"falseCount", 0}});
          auto priceRanges =
              calculatePriceRanges(filteredOffers, priceRangeWidth);
          auto carTypeCounts = calculateCarTypeCounts(filteredOffers);
          auto seatsCount = calculateSeatsCount(filteredOffers);
          auto freeKilometerRanges = calculateFreeKilometerRanges(
              filteredOffers, minFreeKilometerWidth);
          auto vollkaskoCounts = calculateVollkaskoCounts(filteredOffers);

          // Convert aggregations to JSON
          std::vector<crow::json::wvalue> priceRangesJson;
          for (const auto &range : priceRanges) {
            crow::json::wvalue rangeJson;
            rangeJson["start"] = range.start;
            rangeJson["end"] = range.end;
            rangeJson["count"] = range.count;
            priceRangesJson.push_back(std::move(rangeJson));
          }

          std::vector<crow::json::wvalue> seatsCountJson;
          for (const auto &sc : seatsCount) {
            crow::json::wvalue seatsJson;
            seatsJson["numberSeats"] = sc.numberSeats;
            seatsJson["count"] = sc.count;
            seatsCountJson.push_back(std::move(seatsJson));
          }

          std::vector<crow::json::wvalue> freeKmRangesJson;
          for (const auto &range : freeKilometerRanges) {
            crow::json::wvalue rangeJson;
            rangeJson["start"] = range.start;
            rangeJson["end"] = range.end;
            rangeJson["count"] = range.count;
            freeKmRangesJson.push_back(std::move(rangeJson));
          }

          // Prepare offers for response
          std::vector<crow::json::wvalue> resultOffers;
          for (size_t i = startIdx; i < endIdx; i++) {
            crow::json::wvalue offerJson;
            offerJson["ID"] = filteredOffers[i].id;
            offerJson["data"] = filteredOffers[i].data;
            resultOffers.push_back(std::move(offerJson));
          }

          // Construct the response exactly matching the OpenAPI spec
          crow::json::wvalue response;
          response["offers"] = std::move(resultOffers);
          response["priceRanges"] = std::move(priceRangesJson);
          response["carTypeCounts"] =
              crow::json::wvalue({{"small", carTypeCounts.small},
                                  {"sports", carTypeCounts.sports},
                                  {"luxury", carTypeCounts.luxury},
                                  {"family", carTypeCounts.family}});
          response["seatsCount"] = std::move(seatsCountJson);
          response["freeKilometerRange"] = std::move(freeKmRangesJson);
          response["vollkaskoCount"] =
              crow::json::wvalue({{"trueCount", vollkaskoCounts.trueCount},
                                  {"falseCount", vollkaskoCounts.falseCount}});

          res.write(response.dump());
          res.end();
        } catch (const std::exception &e) {
          res.code = 400;
          res.write("Invalid parameters");
          res.end();
        }
      });

  // DELETE /api/offers - Delete all offers
  CROW_ROUTE(app, "/api/offers")
      .methods("DELETE"_method)(
          [](const crow::request &req, crow::response &res) {
            std::lock_guard<std::mutex> lock(offers_mutex);
            offers.clear();
            res.code = 200;
            res.end();
          });

  // TODO:
  app.port(80).multithreaded().run();

  return 0;
}
