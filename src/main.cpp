#include "crow_all.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

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
                                             uint32_t priceRangeWidth,
                                             optional<uint16_t> minPrice,
                                             optional<uint16_t> maxPrice) {
  if (offers.empty() || priceRangeWidth == 0) {
    return {};
  }

  vector<Offer> sortedOffers(offers);
  sort(sortedOffers.begin(), sortedOffers.end(),
       [](const Offer &a, const Offer &b) { return a.price < b.price; });
  // Create a map to store counts for each bucket
  std::map<uint16_t, uint32_t> bucketCounts;

  // Find actual min and max prices from the offers
  uint16_t actualMinPrice = UINT16_MAX;
  uint16_t actualMaxPrice = 0;

  // Count offers in each bucket and track min/max prices
  for (const auto &offer : sortedOffers) {
    // Skip if outside optional price range filters
    if (minPrice && offer.price < *minPrice)
      continue;
    if (maxPrice && offer.price >= *maxPrice)
      continue;

    // Calculate bucket start by rounding down to nearest multiple of width
    uint16_t bucketStart = (offer.price / priceRangeWidth) * priceRangeWidth;

    bucketCounts[bucketStart]++;

    actualMinPrice = std::min(actualMinPrice, offer.price);
    actualMaxPrice = std::max(actualMaxPrice, offer.price);
  }

  // Convert buckets to ranges
  std::vector<PriceRange> ranges;
  for (const auto &[bucketStart, count] : bucketCounts) {
    uint16_t bucketEnd = bucketStart + priceRangeWidth;
    ranges.push_back({
        bucketStart, // Start of range
        bucketEnd,   // End of range
        count        // Number of offers in this range
    });
  }

  // Sort ranges by start price
  std::sort(ranges.begin(), ranges.end(),
            [](const PriceRange &a, const PriceRange &b) {
              return a.start < b.start;
            });

  return ranges;
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
                             uint32_t minFreeKilometerWidth,
                             optional<uint16_t> minFreeKilometer) {
  if (offers.empty() || minFreeKilometerWidth == 0) {
    return {};
  }

  // Sort offers by free kilometers
  std::vector<Offer> sortedOffers(offers);
  std::sort(sortedOffers.begin(), sortedOffers.end(),
            [](const Offer &a, const Offer &b) {
              return a.freeKilometers < b.freeKilometers;
            });

  // Create map to count offers in each bucket
  std::map<uint16_t, uint32_t> bucketCounts;

  // Calculate bucket start for each offer and count
  for (const auto &offer : sortedOffers) {
    // Skip offers below minFreeKilometer if specified
    if (minFreeKilometer && offer.freeKilometers < *minFreeKilometer) {
      continue;
    }

    // Calculate bucket start by rounding down to nearest multiple of width
    uint16_t bucketStart =
        (offer.freeKilometers / minFreeKilometerWidth) * minFreeKilometerWidth;
    bucketCounts[bucketStart]++;
  }

  // Convert buckets to ranges
  std::vector<FreeKilometerRange> ranges;
  for (const auto &[bucketStart, count] : bucketCounts) {
    uint16_t bucketEnd = bucketStart + minFreeKilometerWidth;
    ranges.push_back({
        bucketStart, // Start of range
        bucketEnd,   // End of range
        count        // Number of offers in this range
    });
  }

  return ranges;
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
std::unordered_map<int32_t, std::set<int32_t>> regionToSubregions;

// Helper functions
bool isValidCarType(const std::string &type) {
  return type == "small" || type == "sports" || type == "luxury" ||
         type == "family";
}

void processRegion(const crow::json::rvalue &region,
                   std::unordered_map<int32_t, std::set<int32_t>> &regions) {
  int32_t regionId = region["id"].i();

  // Add the region itself to its own subregions set
  regions[regionId].insert(regionId); // Add this line

  if (region.has("subregions")) {
    for (const auto &subregion : region["subregions"]) {
      int32_t subregionId = subregion["id"].i();
      regions[regionId].insert(subregionId);

      // Process subregion recursively
      processRegion(subregion, regions);

      // Add all subregions of the subregion to the current region
      if (regions.count(subregionId)) {
        regions[regionId].insert(regions[subregionId].begin(),
                                 regions[subregionId].end());
      }
    }
  }
}

void loadRegions() {
  std::ifstream f("regions.json");
  if (!f.is_open()) {
    throw std::runtime_error("Could not open regions.json");
  }

  std::string content((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());

  auto data = crow::json::load(content);
  if (!data) {
    throw std::runtime_error("Failed to parse regions.json");
  }

  processRegion(data, regionToSubregions);
}

int main() {
  try {
    loadRegions();
  } catch (const std::exception &e) {
    std::cerr << "Failed to load regions: " << e.what() << std::endl;
    return 1;
  }

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

        std::cout << "now stored offers\n";
        for (Offer &o : offers) {
          std::cout << o.id << ", ";
        }
        std::cout << "\n";

        res.code = 200;
        res.end();
      });

  // GET /api/offers - Search offers
  CROW_ROUTE(app, "/api/offers")
      .methods("GET"_method)([](const crow::request &req) {
        try {

          // Parse mandatory parameters
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

          std::optional<uint16_t> minFreeKilometer;
          if (req.url_params.get("minFreeKilometer") != nullptr) {
            minFreeKilometer =
                std::stoi(req.url_params.get("minFreeKilometer"));
          }

          // Parse optional parameters
          std::optional<uint8_t> minNumberSeats;
          if (req.url_params.get("minNumberSeats") != nullptr) {
            minNumberSeats = std::stoi(req.url_params.get("minNumberSeats"));
          }

          std::optional<uint16_t> minPrice;
          if (req.url_params.get("minPrice") != nullptr) {
            minPrice = std::stoi(req.url_params.get("minPrice"));
          }

          std::optional<uint16_t> maxPrice;
          if (req.url_params.get("maxPrice") != nullptr) {
            maxPrice = std::stoi(req.url_params.get("maxPrice"));
          }

          std::optional<std::string> carType;
          if (req.url_params.get("carType") != nullptr) {
            carType = req.url_params.get("carType");
          }

          std::optional<bool> onlyVollkasko;
          if (req.url_params.get("onlyVollkasko") != nullptr) {
            onlyVollkasko = req.url_params.get("onlyVollkasko") == "true";
          }

          // Filter offers based on parameters
        std::vector<Offer> filteredOffers;
        std::vector<Offer> filteredOffersExceptPrice;
        std::vector<Offer> filteredOffersExceptCarType;
        std::vector<Offer> filteredOffersExceptFreeKilometers;
        std::vector<Offer> filteredOffersExceptSeatsCount;
        std::vector<Offer> filteredOffersExceptVollkasko;
        {
          std::lock_guard<std::mutex> lock(offers_mutex);

          auto validRegions = regionToSubregions[regionID];
          validRegions.insert(regionID); // include the region itself

          for (const auto &offer : offers) {
            // Check region
            if (validRegions.count(offer.mostSpecificRegionID) == 0) {
              continue;
            }

            // Apply mandatory filters
            // Check if offer overlaps with the requested time range
            if (offer.endDate < timeRangeStart || offer.startDate > timeRangeEnd) {
              continue;
            }

            // Common filters for all vectors
            bool passesMinNumberSeats = !(minNumberSeats && offer.numberSeats < *minNumberSeats);
            bool passesOnlyVollkasko = !(onlyVollkasko && !offer.hasVollkasko);

            // General filter including all criteria
            if (passesMinNumberSeats && passesOnlyVollkasko &&
                (!minPrice || offer.price >= *minPrice) &&
                (!maxPrice || offer.price < *maxPrice) &&
                (!carType || offer.carType == *carType) &&
                (!minFreeKilometer || offer.freeKilometers >= *minFreeKilometer)) {
              filteredOffers.push_back(offer);
            }

            // Filter excluding price
            if (passesMinNumberSeats && passesOnlyVollkasko && 
                (!carType || offer.carType == *carType) &&
                (!minFreeKilometer || offer.freeKilometers >= *minFreeKilometer)) {
              filteredOffersExceptPrice.push_back(offer);
            }

            // Filter excluding carType
            if (passesMinNumberSeats && passesOnlyVollkasko && 
                (!minPrice || offer.price >= *minPrice) &&
                (!maxPrice || offer.price < *maxPrice) &&
                (!minFreeKilometer || offer.freeKilometers >= *minFreeKilometer)) {
              filteredOffersExceptCarType.push_back(offer);
            }

            // Filter excluding freeKilometers
            if (passesMinNumberSeats && passesOnlyVollkasko && 
                (!minPrice || offer.price >= *minPrice) &&
                (!maxPrice || offer.price < *maxPrice) &&
                (!carType || offer.carType == *carType)) {
              filteredOffersExceptFreeKilometers.push_back(offer);
            }

            // Filter excluding seats count
            if (passesOnlyVollkasko &&
                (!minPrice || offer.price >= *minPrice) &&
                (!maxPrice || offer.price < *maxPrice) &&
                (!carType || offer.carType == *carType) &&
                (!minFreeKilometer || offer.freeKilometers >= *minFreeKilometer)) {
              filteredOffersExceptSeatsCount.push_back(offer);
            }

            // Filter excluding vollkasko
            if (passesMinNumberSeats &&
                (!minPrice || offer.price >= *minPrice) &&
                (!maxPrice || offer.price < *maxPrice) &&
                (!carType || offer.carType == *carType) &&
                (!minFreeKilometer || offer.freeKilometers >= *minFreeKilometer)) {
              filteredOffersExceptVollkasko.push_back(offer);
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
          cout << "we ball" << endl;

          // Calculate aggregations
          auto priceRanges = calculatePriceRanges(
              filteredOffersExceptPrice, priceRangeWidth, minPrice, maxPrice);
          cout << "baller" << endl;
          auto carTypeCounts =
              calculateCarTypeCounts(filteredOffersExceptCarType);
          auto seatsCount = calculateSeatsCount(filteredOffersExceptSeatsCount);
          auto freeKilometerRanges = calculateFreeKilometerRanges(
              filteredOffersExceptFreeKilometers, minFreeKilometerWidth,
              minFreeKilometer);
          auto vollkaskoCounts = calculateVollkaskoCounts(filteredOffersExceptVollkasko);

          cout << "we ball even harder" << endl;
          // Paginate results
          size_t startIdx = page * pageSize;
          size_t endIdx = std::min(startIdx + pageSize, filteredOffers.size());

          // Prepare response JSON
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
          if (startIdx < filteredOffers.size()) {
            for (size_t i = startIdx; i < endIdx; i++) {
              crow::json::wvalue offerJson;
              offerJson["ID"] = filteredOffers[i].id;
              offerJson["data"] = filteredOffers[i].data;
              resultOffers.push_back(std::move(offerJson));
            }
          }

          // Construct final response
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

          return crow::response(200, response);
        } catch (const std::exception &e) {
          // Create an error response
          crow::json::wvalue error_response({{"status", "error"},
                                             {"message", "Invalid parameters"},
                                             {"error", e.what()}});

          // Return error response with status code 400
          return crow::response(400, error_response);
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

  app.port(80).multithreaded().run();

  return 0;
}
