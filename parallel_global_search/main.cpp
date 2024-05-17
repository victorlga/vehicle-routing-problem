#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <omp.h>
#include <mpi.h>

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
        std::map<Place, Load>& placesDemand,
        int world_rank,
        int world_size
    ) : numberOfPlaces(numberOfPlaces), vehicleCapacity(vehicleCapacity), maxNumberOfPlacesPerRoute(maxNumberOfPlacesPerRoute),
        roads(roads), placesDemand(placesDemand), world_rank(world_rank), world_size(world_size) {}

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

        Route localBestRoute = bestRoute;

        for(auto& route : routes)
        {
            if (route.cost < bestRoute.cost)
                localBestRoute = route;
        }

        // Reduce the best route from all processes
        MPI_Allreduce(&localBestRoute.cost, &bestRoute.cost, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    }

    private:
    int numberOfPlaces;
    int vehicleCapacity;
    int maxNumberOfPlacesPerRoute;
    std::map<Place, std::map<Place, Cost>> roads;
    std::vector<Route> routes;
    std::map<Place, Load>& placesDemand;
    int world_rank;
    int world_size;

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

            if (world_rank >= numberOfPlaces)
                continue;

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

            // Filter route sections that are not part of the current process
            if (route.places.size() > 1 && route.places[1] != world_rank)
                continue;
            else
                std::cout << route.places[1] << ' ' << world_rank << std::endl;

            route.cost += roads[previousPlace][currentPlace];
            placesVisited.insert(currentPlace);
            route.places.push_back(currentPlace);

            if (currentPlace == 0)
            {
                if (placesVisited.size() == numberOfPlaces)
                {
                    #pragma omp critical
                    routes.push_back(route);
                    return;
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


int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

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
            std::cerr << "Error opening file: " << fileNames[j] << std::endl;
            continue;
        }

        std::string line;
        getline(file, line);
        int numberOfPlaces = std::stoi(line);

        std::map<Place, Load> placesDemand;
        placesDemand[0] = 0; // Consider place 0

        for (int i = 0; i < numberOfPlaces; ++i)
        {
            getline(file, line);
            std::istringstream iss(line);
            int place;
            Load demand;
            iss >> place >> demand;
            placesDemand[place] = demand;
        }

        numberOfPlaces++; // Increment to consider place 0

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
        int maxNumberOfPlacesPerRoute = 3;

        CapacitatedVehicleRoutingProblem CVRP(
            numberOfPlaces,
            vehicleCapacity,
            maxNumberOfPlacesPerRoute,
            roads,
            placesDemand,
            world_rank,
            world_size
        );

        CVRP.solve();

        Route bestRoute = CVRP.bestRoute;

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        if (world_rank == 0)
        {
            std::cout << "Running solution for " << fileNames[j] << std::endl;
            std::cout << "Best route Place sequence: ";
            for (Place& place : bestRoute.places) std::cout << place << " -> ";
            std::cout << std::endl;
            std::cout << "Best route cost: " << bestRoute.cost << std::endl;
            std::cout << "Time taken: " << duration << " ms" << std::endl;
            std::cout << "--------------------------------------------------------" << std::endl;
        }
    }

    MPI_Finalize();

    return 0;
}