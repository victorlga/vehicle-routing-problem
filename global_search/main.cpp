#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <string>

using Place = int;
using Load = int;
using Cost = int;
using Route = std::vector<Place>;

struct Road
{
    Place source;
    Place destination;
    Cost cost;

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
        generateAllRouteCombinationsWithRestrictions(placesVisited, 0, 0, 0, route);

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

            if (placesVisited.find(currentPlace) != placesVisited.end() && currentPlace != 0)
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

            if (currentPlace != 0)
            {
                bool loadExceeded = vehicleLoad+placeDemand.second > vehicleCapacity;
                bool placesExceeded = numberOfPlacesVisited+1 > maxNumberOfPlacesPerRoute;
                if (loadExceeded || placesExceeded) continue;
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
    std::vector<std::string> fileNames = {"../graphs/graph0.txt", "../graphs/graph1.txt", "../graphs/graph2.txt", "../graphs/graph3.txt"};
    for (int j = 0; j < 4; ++j)
    {
        std::ifstream file(fileNames[j]);
        if (!file.is_open())
        {
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

        std::cout << "Running solution for " << fileNames[j] << std::endl;
        std::cout << "Best route Place sequence: ";
        for (Place& place : bestRoute) std::cout << place << " -> ";
        std::cout << std::endl << "Best route cost: " << lowerCost << std::endl;
        std::cout << "--------------------------------------------------------" << std::endl;
    }
}
