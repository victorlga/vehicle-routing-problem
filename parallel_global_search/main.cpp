#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <string>
#include <omp.h>

using Place = int;
using Load = int;
using Cost = int;

struct Route
{
    std::vector<Place> places;
    Cost cost;

    Route(std::vector<Place> places, Cost cost) : places(places), cost(cost) {}
};

struct Road
{
    Place source;
    Place destination;
    Cost cost;

    Road(Place source, Place destination, Cost cost) : source(source), destination(destination), cost(cost) {}
};

class CapacitatedVehicleRoutingProblem
{
    public:
    Route bestRoute = Route({}, INT_MAX);

    CapacitatedVehicleRoutingProblem(
        int numberOfPlaces,
        int vehicleCapacity,
        int maxNumberOfPlacesPerRoute,
        std::map<Place, std::map<Place, Cost>> roads,
        std::map<Place, Load>& placesDemand
    ) : numberOfPlaces(numberOfPlaces), vehicleCapacity(vehicleCapacity), maxNumberOfPlacesPerRoute(maxNumberOfPlacesPerRoute), roads(roads), placesDemand(placesDemand) {}

    void solve()
    {
        std::set<Place> placesVisited;
        placesVisited.insert(0);
        Route route = Route({0}, 0);

        #pragma omp parallel
        {
            #pragma omp single
            {
                generateAllRouteCombinationsWithRestrictions(placesVisited, 0, 0, 0, route);
            }
        }

        for(auto& route : routes)
        {
            if (route.cost < bestRoute.cost)
                bestRoute = route;
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

            // Filter repeated places
            if (currentPlace == previousPlace)
                continue;

            // Filter unavailable roads
            if (roads[previousPlace].find(currentPlace) == roads[previousPlace].end())
                continue;

            // Filter places already visited
            if (placesVisited.find(currentPlace) != placesVisited.end() && currentPlace != 0)
                continue;

            if (currentPlace != 0)
            {
                bool loadExceeded = (vehicleLoad+placeDemand.second) > vehicleCapacity;
                bool placesExceeded = (numberOfPlacesVisited+1) > maxNumberOfPlacesPerRoute;
                if (loadExceeded || placesExceeded)
                    continue;
            }

            route.cost += roads[previousPlace][currentPlace];
            placesVisited.insert(currentPlace);
            route.places.push_back(currentPlace);

            if (currentPlace == 0)
            {
                if (placesVisited.size() == numberOfPlaces)
                {
                    #pragma omp critical
                    routes.push_back(route);
                }
                #pragma omp task
                generateAllRouteCombinationsWithRestrictions(placesVisited, 0, currentPlace, 0, route);
            } else {
                #pragma omp task
                generateAllRouteCombinationsWithRestrictions(
                    placesVisited,
                    numberOfPlacesVisited+1,
                    currentPlace,
                    vehicleLoad+placeDemand.second,
                    route
                );
            }

            route.places.pop_back();
            placesVisited.extract(currentPlace);
            route.cost -= roads[previousPlace][currentPlace];
        }
    }
};


int main()
{
    std::vector<std::string> fileNames = {
        "../graphs/graph0.txt",
        "../graphs/graph1.txt",
        "../graphs/graph2.txt",
        "../graphs/graph3.txt",
    };

    for (int j = 0; j < fileNames.size(); ++j)
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

        Load vehicleCapacity = 20;
        int maxNumberOfPlacesPerRoute = 2;

        CapacitatedVehicleRoutingProblem CVRP = CapacitatedVehicleRoutingProblem(
            numberOfPlaces,
            vehicleCapacity,
            maxNumberOfPlacesPerRoute,
            roads,
            placesDemand
        );

        CVRP.solve();

        Route bestRoute = CVRP.bestRoute;

        std::cout << "Running solution for " << fileNames[j] << std::endl;
        std::cout << "Best route Place sequence: ";
        for (Place& place : bestRoute.places) std::cout << place << " -> ";
        std::cout << std::endl << "Best route cost: " << bestRoute.cost << std::endl;
        std::cout << "--------------------------------------------------------" << std::endl;
    }
}
