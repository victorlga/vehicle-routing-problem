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


struct Road
{
    int id;
    Place source;
    Place destination;
    Cost cost;

    bool operator<(const Road& other) const
    {
        return id < other.id;
    }

    Road(int id, Place source, Place destination, Cost cost) : id(id), source(source), destination(destination), cost(cost) {}
};

using Route = std::vector<Road>;

void generateAllRoutes(std::vector<Route>& routes, Route route, std::vector<Road>& roads, int roadIndex)
{
    if (roadIndex >= roads.size())
    {
        sort(route.begin(), route.end());
        do
        {
            if (route.size() > 1) routes.push_back(route);
        } while (next_permutation(route.begin(), route.end()));
        return;
    }

    generateAllRoutes(routes, route, roads, roadIndex+1);

    route.push_back(roads[roadIndex]);
    generateAllRoutes(routes, route, roads, roadIndex+1);
    route.pop_back();
}

bool checkSourceDestinationIs0(Route& route)
{
    Road firstRoad = route.front();
    size_t lastRouteIndex = route.size() - 1;
    Road lastRoad = route[lastRouteIndex];

    return firstRoad.source == 0 && lastRoad.destination == 0;
}

bool checkAllDemands(Route& route, int numberOfPlaces)
{
    std::set<Place> placesAttended;

    for (Road& road : route)
        placesAttended.insert(road.destination);

    return placesAttended.size() == numberOfPlaces;
}

bool checkVehicleCapacity(Route& route, std::map<Place, Load> placesDemand, Load vehicleCapacity)
{
    Load totalLoad = 0;
    for (Road& road : route)
    {
        totalLoad += placesDemand[road.destination];
        if (road.destination == 0)
        {
            if (totalLoad > vehicleCapacity) return false;
            totalLoad = 0;
        }
    }
    return true;
}

bool checkMaxNumberOfPlacesVisited(Route& route, int maxNumberOfPlacesAllowed)
{
    int numberOfPlacesVisited = 0;
    for (Road& road : route)
    {
        numberOfPlacesVisited += 1;
        if (road.destination == 0)
        {
            if (numberOfPlacesVisited > maxNumberOfPlacesAllowed) return false;
            numberOfPlacesVisited = 0;
        }
    }
    return true;
}

bool checkAllRestrictions(Route& route, std::map<Place, Load>& placesDemand, Load vehicleCapacity, int maxNumberOfPlacesAllowed)
{
    if (checkSourceDestinationIs0(route))
    {
        if (checkAllDemands(route, placesDemand.size()))
        {
            if (checkVehicleCapacity(route, placesDemand, vehicleCapacity))
            {
                if (checkMaxNumberOfPlacesVisited(route, maxNumberOfPlacesAllowed))
                    return true;
            }
        }
    }

    return false;
}

std::vector<Route> filterRoutes(std::vector<Route>& routes, std::map<Place, Load> placesDemand, Load vehicleCapacity, int maxNumberOfPlacesAllowed)
{
    std::vector<Route> validRoutes;

    for (Route& route : routes)
    {
        if (checkAllRestrictions(route, placesDemand, vehicleCapacity, maxNumberOfPlacesAllowed))
            validRoutes.push_back(route);
    }

    return validRoutes;
}

Cost calculateRouteCost(Route& route)
{
    Cost totalCost = 0;
    for (Road& road : route)
    {
        totalCost += road.cost;
    }
 
    return totalCost;
}

std::pair<Route, Cost> solveVRPWithDemand(std::map<Place, Load>& placesDemand, std::vector<Road>& roads, int vehicleCapacity, int maxNumberOfPlacesAllowed)
{
    Route bestRoute;
    Cost lowerCost = INT_MAX;

    std::vector<Route> routes;
    generateAllRoutes(routes, Route(), roads, 0);

    std::cout << "Teste" << std::endl;

    std::vector<Route> validRoutes = filterRoutes(routes, placesDemand, vehicleCapacity, maxNumberOfPlacesAllowed);

    for(Route& route : validRoutes)
    {
        Cost cost = calculateRouteCost(route);
        if (cost < lowerCost)
        {
            lowerCost = cost;
            bestRoute = route;
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

    getline(file, line);
    int numberOfRoads = std::stoi(line);

    std::vector<Road> roads;

    for (int roadId = 0; roadId < numberOfRoads; ++roadId)
    {
        getline(file, line);
        std::istringstream iss(line);
        Place source;
        Place destination;
        Cost cost;

        iss >> source >> destination >> cost;
        roads.push_back(Road(roadId, source, destination, cost));
    }

    Load vehicleCapacity = 30;
    int maxNumberOfPlacesAllowed = 4;

    std::pair<Route, Cost> solution = solveVRPWithDemand(placesDemand, roads, vehicleCapacity, maxNumberOfPlacesAllowed);

    Route bestRoute = solution.first;
    Cost lowerCost = solution.second;

    std::cout << "Best route Place sequence: ";
    for (Road& road : bestRoute) std::cout << road.source << " -> ";

    int lastRoadIndex = bestRoute.size() - 1;
    std::cout << bestRoute[lastRoadIndex].destination << std::endl << std::endl;

    std::cout << "Best route cost: " << lowerCost << std::endl;
}
