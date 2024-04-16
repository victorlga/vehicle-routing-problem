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
    Cost lowerCost;

    VehicleRoutingProblemWithDemand(
        int numberOfPlaces,
        int vehicleCapacity,
        int maxNumberOfPlacesPerRoute,
        std::map<Place, Load>& placesDemand,
        std::map<Place, std::map<Place, Cost>> roads
    ) : placesDemand(placesDemand), vehicleCapacity(vehicleCapacity), maxNumberOfPlacesPerRoute(maxNumberOfPlacesPerRoute), numberOfPlaces(numberOfPlaces), roads(roads) {}

    void solve()
    {
        placesVisited.insert(0);
        bestRoute.push_back(0);
        lowerCost = 0;
        int numberOfPlacesVisited = 0;
        Load vehicleLoad = 0;

        int routePlaceIndex = 0;
        Place currentPlace = bestRoute[routePlaceIndex];
        while (placesVisited.size() < numberOfPlaces)
        {
            std::map<Place, Cost> availableRoads = roads[currentPlace];
            std::pair<Place, Cost> nextRoad = findCheaperValidRoad(availableRoads, numberOfPlacesVisited, vehicleLoad);
            placesVisited.insert(nextRoad.first);
            bestRoute.push_back(nextRoad.first);
            lowerCost += nextRoad.second;
            vehicleLoad += placesDemand[nextRoad.first];
            routePlaceIndex++;
            currentPlace = bestRoute[routePlaceIndex];
        }

        bestRoute.push_back(0);
        lowerCost += roads[bestRoute.back()][0];
    }

    private:
    int numberOfPlaces;
    int vehicleCapacity;
    int maxNumberOfPlacesPerRoute;
    std::set<Place> placesVisited;
    std::map<Place, std::map<Place, Cost>> roads;
    std::map<Place, Load>& placesDemand;

    std::pair<Place, Cost> findCheaperValidRoad(std::map<Place, Cost> availableRoads, Cost& numberOfPlacesVisited, Load& vehicleLoad)
    {
        std::pair<Place, Cost> cheaperRoad(INT_MAX, INT_MAX);
        for (auto const& place : availableRoads)
        {

            bool newPlaceOrStart = (placesVisited.find(place.first) == placesVisited.end() || place.first == 0);
            if (place.second < cheaperRoad.second && newPlaceOrStart)
                cheaperRoad = place;
        }

        numberOfPlacesVisited++;
        vehicleLoad += placesDemand[cheaperRoad.first];

    
        if (vehicleLoad > vehicleCapacity || numberOfPlacesVisited > maxNumberOfPlacesPerRoute)
            cheaperRoad = std::pair<Place, Cost>(0, availableRoads[0]);
        
        if (cheaperRoad.first == 0)
        {
            numberOfPlacesVisited = 0;
            vehicleLoad = 0;
        }

        return cheaperRoad;
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
            placesDemand,
            roads
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