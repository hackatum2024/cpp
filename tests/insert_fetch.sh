# Create offer
curl -X POST -H "Content-Type: application/json" -d '{
    "offers": [{
        "ID": "01934a57-7988-7879-bb9b-e03bd4e77b9d",
        "data": "SGVsbG8gV29ybGQ=",
        "mostSpecificRegionID": 5,
        "startDate": 1732104000000,
        "endDate": 1732449600000,
        "numberSeats": 5,
        "price": 10000,
        "carType": "luxury",
        "hasVollkasko": true,
        "freeKilometers": 120
    }]
}' http://localhost/api/offers

# Get offers
curl "http://localhost/api/offers?regionID=5&timeRangeStart=1732104000000&timeRangeEnd=1732449600000&numberDays=7&sortOrder=price-asc&page=0&pageSize=10&priceRangeWidth=1000&minFreeKilometerWidth=50"


