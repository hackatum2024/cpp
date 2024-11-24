# C++ REST API for Car Rental Comparison

Implementation of a REST API in C++ using Crow.
The implementation has 3 endpoints:
- a POST endpoint where we take new car rental offers and store it internally
- a GET endpoint to query and filter the offers according to request; it also returns aggregations of for example the number cars in a certain price category
- a DELETE endpoint to completely delete all stored offers

It is our submission for the CHECK24 challenge in the hackaTUM.
