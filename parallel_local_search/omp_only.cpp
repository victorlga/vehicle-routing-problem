#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <string>
#include <random>
#include <omp.h>
#include <chrono>

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

class CapacitatedVehicleRoutingProblem
{
    public:
    Route bestRoute;
    Cost lowerCost = INT_MAX;

    CapacitatedVehicleRoutingProblem(
        int numberOfPlaces,
        int vehicleCapacity,
        int maxNumberOfPlacesPerRoute,
        std::map<Place, Load> placesDemand,
        std::map<Place, std::map<Place, Cost>> roads
    ) : numberOfPlaces(numberOfPlaces), vehicleCapacity(vehicleCapacity), maxNumberOfPlacesPerRoute(maxNumberOfPlacesPerRoute), placesDemand(placesDemand), roads(roads) {}

    void solve()
    {
        #pragma omp parallel private()
        {
            Route localBestRoute;
            Cost localLowerCost = INT_MAX;
            #pragma omp for
            for (int i = 0; i < 10000; ++i)
            {
                std::pair<Route, Cost> result = generateRouteAndCost();
                if (result.second < lowerCost)
                {
                    localBestRoute = result.first;
                    localLowerCost = result.second;
                }
            }

            #pragma omp critical
            {
                if (localLowerCost < lowerCost)
                {
                    bestRoute = localBestRoute;
                    lowerCost = localLowerCost;
                }
            }
        }
    }

    private:
    int numberOfPlaces;
    int vehicleCapacity;
    int maxNumberOfPlacesPerRoute;
    std::map<Place, Load> placesDemand;
    std::map<Place, std::map<Place, Cost>> roads;

    std::pair<Route, Cost> generateRouteAndCost()
    {
        std::set<Place> placesVisited;
        placesVisited.insert(0);
        Route route = {0};
        Cost cost = 0;
        int numberOfPlacesVisited = 0;
        Load vehicleLoad = 0;

        int routePlaceIndex = 0;
        Place currentPlace = route[routePlaceIndex];
        std::map<Place, Cost> availableRoads;
        std::pair<Place, Cost> nextRoad;

        while (placesVisited.size() < numberOfPlaces)
        {
            availableRoads = roads[currentPlace];
            nextRoad = findCheaperValidRoad(availableRoads, numberOfPlacesVisited, vehicleLoad, placesVisited, currentPlace);
            placesVisited.insert(nextRoad.first);
            route.push_back(nextRoad.first);
            cost += nextRoad.second;
            vehicleLoad += placesDemand[nextRoad.first];
            routePlaceIndex++;
            currentPlace = route[routePlaceIndex];
        }

        cost += roads[route.back()][0];
        route.push_back(0);

        return std::pair<Route, Cost>(route, cost);
    }

    std::pair<Place, Cost> findCheaperValidRoad(std::map<Place, Cost> availableRoads, int& numberOfPlacesVisited, Load& vehicleLoad, std::set<Place>& placesVisited, Place previousPlace)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> uniformRealDistr(0.0, 1.0);
        std::uniform_int_distribution<> uniformIntDistr(0, INT_MAX);

        std::pair<Place, Cost> cheaperRoad(INT_MAX, INT_MAX);

        for (auto const& road : availableRoads)
        {
            bool newPlaceOrStart = (placesVisited.find(road.first) == placesVisited.end() || road.first == 0);
            if (road.second < cheaperRoad.second && newPlaceOrStart && road.first != previousPlace)
                cheaperRoad = road;
        }

        if (uniformRealDistr(gen) > 0.5)
        {
            std::vector<Place> availablePlaces;
            for (const auto& road : availableRoads)
                availablePlaces.push_back(road.first);

            int randomPlaceIndex = uniformIntDistr(gen) % availableRoads.size();
            int randomPlace = availablePlaces[randomPlaceIndex];

            if (placesVisited.find(randomPlace) == placesVisited.end() || (randomPlace == 0 && randomPlace != previousPlace))
                cheaperRoad = std::pair<Place, Cost>(randomPlace, availableRoads[randomPlace]);
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
    std::vector<std::string> fileNames = {
        "../graphs/graph4_50.txt",
        "../graphs/graph5_50.txt",
        "../graphs/graph6_50.txt",
        "../graphs/graph7_50.txt",
        "../graphs/graph8_50.txt",
        "../graphs/graph9_50.txt",
        "../graphs/graph10_50.txt",
    };

    for (int j = 0; j < fileNames.size(); ++j)
    {
        auto startTime = std::chrono::high_resolution_clock::now();
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

        Load vehicleCapacity = 10;
        int maxNumberOfPlacesPerRoute = 4;

        CapacitatedVehicleRoutingProblem CVRP = CapacitatedVehicleRoutingProblem(
            numberOfPlaces,
            vehicleCapacity,
            maxNumberOfPlacesPerRoute,
            placesDemand,
            roads
        );

        CVRP.solve();

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        Route bestRoute = CVRP.bestRoute;
        Cost lowerCost = CVRP.lowerCost;

        std::cout << "Running solution for " << fileNames[j] << std::endl;
        std::cout << "Best route Place sequence: ";
        for (Place &place : bestRoute) {
            std::cout << place << " -> ";
        }
        std::cout << std::endl << "Best route cost: " << lowerCost << std::endl;
        std::cout << "Time taken: " << duration.count() << " ms" << std::endl;
        std::cout << "--------------------------------------------------------" << std::endl;
    }

    return 0;
}