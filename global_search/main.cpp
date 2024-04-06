#include <algorithm>
#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

using Place = int;
using Load = int;
using Cost = int;
using Route = std::vector<Place>;

void generateAllRoutes(std::vector<Route>& routes, Route route, std::vector<Place>& places, int placeIndex)
{
    if (placeIndex == places.size())
    {
        sort(route.begin(), route.end());
        do
        {
            if (route.size() > 1) routes.push_back(route);
        } while (next_permutation(route.begin(), route.end()));
        return;
    }

    generateAllRoutes(routes, route, places, placeIndex+1);

    route.push_back(places[placeIndex]);
    generateAllRoutes(routes, route, places, placeIndex+1);
    route.pop_back();
}

bool checkVehicleCapacityFitRoute(Route& route, std::map<Place, Load>& demand, Load capacity)
{
    Load totalLoad = 0;
    for (Place& place : route) totalLoad += demand[place];
    return totalLoad <= capacity;
}

Cost calculateRouteCost(Route& route, std::map<std::pair<Place, Place>, Cost>& roads)
{
    Cost totalCost = 0;
    for (int i = 0; i < route.size() - 1; ++i)
    {
        Place source = route[i];
        Place destination = route[i+1];

        if (roads.count(std::make_pair(source, destination)) == 0)
        {
            totalCost = INT_MAX;
            break;
        }

        totalCost += roads[std::make_pair(source, destination)];
    }

    return totalCost;
}

void updateSolutionIfBetterFound(Cost& currentCost, Cost& lowerCost, Route& route, Route& bestRoute)
{
    if (currentCost < lowerCost)
    {
        lowerCost = currentCost;
        bestRoute = route;
    }
}

std::pair<Route, Cost> solveVRPWithDemand(std::vector<Place> places, std::map<Place, Load> demand, std::map<std::pair<Place, Place>, Cost> roads, int vehicleCapacity)
{
    Route bestRoute;
    Cost lowerCost = INT_MAX;

    std::vector<Route> routes;
    generateAllRoutes(routes, Route(), places, 0);

    for(Route& route : routes)
    {
        if (checkVehicleCapacityFitRoute(route, demand, vehicleCapacity))
        {
            Cost currentCost = calculateRouteCost(route, roads);
            updateSolutionIfBetterFound(currentCost, lowerCost, route, bestRoute);
        }
    }

    return std::make_pair(bestRoute, lowerCost);
}

int main()
{
    std::ifstream file("../graphs/graph0.txt");
    if (!file.is_open()) {
        std::cerr << "Erro na abertura do arquivo ..." << std::endl;
    }

    std::string line;
    getline(file, line);
    int numberOfPlaces = std::stoi(line);

    std::vector<Place> places;
    std::map<Place, Load> demand;

    for (int i = 0; i < numberOfPlaces; ++i)
    {
        getline(file, line);
        std::istringstream iss(line);
        Place place;
        Load load;

        iss >> place >> load;
        places.push_back(place);
        demand[place] = load;
    }

    getline(file, line);
    int numberOfRoads = std::stoi(line);

    std::map<std::pair<Place, Place>, Cost> roads;

    for (int i = 0; i < numberOfRoads; ++i)
    {
        getline(file, line);
        std::istringstream iss(line);
        Place source;
        Place destination;
        Cost cost;

        iss >> source >> destination >> cost;
        roads[std::make_pair(source, destination)] = cost;
    }

    Load vehicleCapacity = 24;

    std::pair<Route, Cost> solution = solveVRPWithDemand(places, demand, roads, vehicleCapacity);

    std::cout << "Place sequence of best route: ";
    for (Place place : solution.first) std::cout << place << ", ";
    std::cout << std::endl << std::endl;

    std::cout << "Best route cost: " << solution.second << std::endl;
}
