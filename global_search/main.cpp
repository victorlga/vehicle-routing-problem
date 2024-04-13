#include <algorithm>
#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

using Place = int;
using Load = int;
using Cost = int;
using Route = std::vector<Place>;

struct Road
{
    Place source;
    Place destination;
    Cost cost;

    // bool operator==(const Road& other) const
    // {
    //     return source == other.source && destination == other.destination;
    // }

    Road(Place source, Place destination, Cost cost) : source(source), destination(destination), cost(cost) {}
};

class VehicleRoutingProblemWithDemand
{
    public:
    Route bestRoute;
    Cost lowerCost = INT_MAX;

    VehicleRoutingProblemWithDemand(
        int numberOfPlaces,
        int vehicleCapacity,
        int maxNumberOfPlacesPerRoute,
        std::map<Place, std::map<Place, Cost>> roads,
        std::map<Place, Load>& placesDemand
    ) : placesDemand(placesDemand), vehicleCapacity(vehicleCapacity), maxNumberOfPlacesPerRoute(maxNumberOfPlacesPerRoute), roads(roads), numberOfPlaces(numberOfPlaces) {}

    void solve()
    {
        std::set<Place> placesVisited;
        placesVisited.insert(0);
        Route route{0};
        std::cout << "Adicionou 0 como primeiro da rota" << std::endl;
        generateAllRouteCombinationsWithRestrictions(placesVisited, 0, 0, 0, route);
        std::cout << "Gerou todas as combinações de rotas" << std::endl;

        std::set<Route> filteredRoutes = filterRoutesWithValidRoads();

        for(auto& route : filteredRoutes)
        {
            Cost cost = calculateRouteCost(route);
            if (cost < lowerCost)
            {
                lowerCost = cost;
                bestRoute = route;
            }
        }
    }

    private:
    int numberOfPlaces;
    int vehicleCapacity;
    int maxNumberOfPlacesPerRoute;
    std::map<Place, std::map<Place, Cost>> roads;
    std::vector<Route> routes;
    std::map<Place, Load>& placesDemand;

    void generateAllRouteCombinationsWithRestrictions(
        std::set<Place> placesVisited,
        int numberOfPlacesVisited,
        Place previousPlace,
        Load vehicleLoad,
        Route route
    )
    {
        for (auto const& placeDemand : placesDemand)
        {
            Place currentPlace = placeDemand.first;
            
            if (currentPlace == previousPlace)
                continue;

            if (currentPlace == 0)
            {
                numberOfPlacesVisited = 0;
                vehicleLoad = 0;
                if (placesVisited.size() == numberOfPlaces)
                {
                    route.push_back(currentPlace);
                    routes.push_back(route);
                    return;
                }
            }
            else {
                bool loadExceeded = vehicleLoad+placeDemand.second > vehicleCapacity;
                bool placesExceeded = numberOfPlacesVisited+1 > maxNumberOfPlacesPerRoute;
                if (loadExceeded || placesExceeded) return;
            }

            placesVisited.insert(currentPlace);
            route.push_back(currentPlace);
            generateAllRouteCombinationsWithRestrictions(placesVisited, numberOfPlacesVisited+1, currentPlace, vehicleLoad+placeDemand.second, route);
            route.pop_back();
            placesVisited.extract(currentPlace);
        }
    }

    std::set<Route> filterRoutesWithValidRoads()
    {
        std::set<Route> filteredRoutes;
        for (Route& route : routes)
        {
            bool routeIsValid = true;
            for (size_t i = 0; i < route.size()-1; ++i)
            {
                Place source = route[i];
                Place destination = route[i+1];
                if (roads[source].find(destination) == roads[source].end())
                {
                    routeIsValid = false;
                    break;
                }
            }

            if (routeIsValid)
                filteredRoutes.insert(route);
        }
        return filteredRoutes;
    }

    Cost calculateRouteCost(Route route)
    {
        Cost totalCost = 0;
        for (size_t i = 0; i < route.size()-1; ++i)
        {
            Place source = route[i];
            Place destination = route[i+1];
            totalCost += roads[source][destination];
        }
        return totalCost;
    }
};


int main()
{
    std::ifstream file("../graphs/graph5.txt");
    if (!file.is_open()) {
        std::cerr << "Erro na abertura do arquivo ..." << std::endl;
    }

    std::string line;
    getline(file, line);
    int numberOfPlaces = std::stoi(line);

    std::map<Place, Load> placesDemand;
    placesDemand[0] = 0;

    for (int i = 0; i < numberOfPlaces; ++i)
    {
        getline(file, line);
        std::istringstream iss(line);
        int place;
        Load demand;

        iss >> place >> demand;
        placesDemand[place] = demand;
    }

    for (int i = 0; i <= numberOfPlaces; ++i)
    {
        std::cout << i << ' ' << placesDemand[i] << std::endl;
    }

    numberOfPlaces++; // To consider place 0

    getline(file, line);
    int numberOfRoads = std::stoi(line);

    std::map<Place, std::map<Place, Cost>> roads;

    for (int roadId = 0; roadId < numberOfRoads; ++roadId)
    {
        getline(file, line);
        std::istringstream iss(line);
        Place source;
        Place destination;
        Cost cost;

        iss >> source >> destination >> cost;
        roads[source][destination] = cost;
    }

    Load vehicleCapacity = 30;
    int maxNumberOfPlacesPerRoute = 4;

    VehicleRoutingProblemWithDemand VRPWithDemand = VehicleRoutingProblemWithDemand(
        numberOfPlaces,
        vehicleCapacity,
        maxNumberOfPlacesPerRoute,
        roads,
        placesDemand
    );

    VRPWithDemand.solve();

    Route bestRoute = VRPWithDemand.bestRoute;
    Cost lowerCost = VRPWithDemand.lowerCost;

    std::cout << "Best route Place sequence: ";
    for (Place& place : bestRoute) std::cout << place << " -> ";
    std::cout << "Best route cost: " << lowerCost << std::endl;
}
