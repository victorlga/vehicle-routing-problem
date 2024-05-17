#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <string>
#include <omp.h>
#include <mpi.h>
#include <chrono>

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

            if (world_rank % numberOfPlaces == currentPlace && placesVisited.size() == 1)
            {
                recursiveCall(placesVisited, numberOfPlacesVisited, previousPlace, vehicleLoad, route, currentPlace, placeDemand);
            } else {
                recursiveCall(placesVisited, numberOfPlacesVisited, previousPlace, vehicleLoad, route, currentPlace, placeDemand);
            }
        }
    }

    void recursiveCall(
        std::set<Place> placesVisited,
        int numberOfPlacesVisited,
        Place previousPlace,
        Load vehicleLoad,
        Route route,
        Place currentPlace,
        std::pair<Place, Load> placeDemand
    )
    {
        route.cost += roads[previousPlace][currentPlace];
        placesVisited.insert(currentPlace);
        route.places.push_back(currentPlace);

        if (currentPlace == 0)
        {
            if (placesVisited.size() == numberOfPlaces)
            {
                #pragma omp critical
                routes.push_back(route);
                if (world_rank == 0)
                {
                    for (int i = 1; i < world_size; ++i)
                    {
                        int routesSize;
                        MPI_Recv(&routesSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        std::vector<int> buffer(routesSize);
                        MPI_Recv(buffer.data(), routesSize, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        auto deserializedRoutes = deserialize(buffer);
                        routes.insert(routes.end(), deserializedRoutes.begin(), deserializedRoutes.end());
                    }
                } else {
                    std::vector<int> flatRoutes = serialize(routes);
                    int routesSize = flatRoutes.size();
                    MPI_Send(&routesSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                    MPI_Send(flatRoutes.data(), routesSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
                }
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

    std::vector<int> serialize(std::vector<Route> routes)
    {
        std::vector<int> flatRoutes;
        for (Route& route : routes)
        {
            flatRoutes.push_back(route.cost);
            for (Place& place : route.places)
            {
                flatRoutes.push_back(place);
            }
            flatRoutes.push_back(-1);
        }
        return flatRoutes;
    }

    std::vector<Route> deserialize(std::vector<int> flatRoutes)
    {
        std::vector<Route> routes;
        int i = 0;
        while (i < flatRoutes.size())
        {
            Cost cost = flatRoutes[i];
            i++;
            std::vector<Place> places;
            while (flatRoutes[i] != -1)
            {
                places.push_back(flatRoutes[i]);
                i++;
            }
            i++;
            routes.push_back(Route(places, cost));
        }
        return routes;
    }
};


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == 0)
        std::cout << "Running solution with " << world_size << " processes" << std::endl;

    std::vector<std::string> fileNames = {
        "../graphs/graph4_50.txt",
        "../graphs/graph5_50.txt",
        "../graphs/graph6_50.txt",
        "../graphs/graph7_50.txt",
        "../graphs/graph8_50.txt",
        "../graphs/graph9_50.txt",
        "../graphs/graph10_50.txt",
    };

    for (int j = 0; j < fileNames.size(); ++j) {
        auto start = std::chrono::high_resolution_clock::now(); // Start time measurement

        std::ifstream file(fileNames[j]);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << fileNames[j] << std::endl;
            continue; // Skip to next file if the current file cannot be opened
        }

        std::string line;
        getline(file, line);
        int numberOfPlaces = std::stoi(line);

        std::map<Place, Load> placesDemand;
        placesDemand[0] = 0; // Assume place 0 as the depot

        for (int i = 0; i < numberOfPlaces; ++i) {
            getline(file, line);
            std::istringstream iss(line);
            Place place;
            Load demand;
            iss >> place >> demand;
            placesDemand[place] = demand;
        }

        numberOfPlaces++; // Including the depot

        getline(file, line);
        int numberOfRoads = std::stoi(line);
        std::map<Place, std::map<Place, Cost>> roads;

        for (int roadId = 0; roadId < numberOfRoads; ++roadId) {
            getline(file, line);
            std::istringstream iss(line);
            Place source, destination;
            Cost cost;
            iss >> source >> destination >> cost;
            roads[source][destination] = cost;
        }

        Load vehicleCapacity = 20;
        int maxNumberOfPlacesPerRoute = 3;

        CapacitatedVehicleRoutingProblem CVRP = CapacitatedVehicleRoutingProblem(
            numberOfPlaces,
            vehicleCapacity,
            maxNumberOfPlacesPerRoute,
            roads,
            placesDemand,
            world_rank,
            world_size
        );

        CVRP.solve();

        if (world_rank == 0) {
            auto end = std::chrono::high_resolution_clock::now(); // End time measurement
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); // Calculate duration in milliseconds

            Route bestRoute = CVRP.bestRoute;
            std::cout << "Running solution for " << fileNames[j] << std::endl;
            std::cout << "Best route Place sequence: ";
            for (Place& place : bestRoute.places) std::cout << place << " -> ";
            std::cout << std::endl << "Best route cost: " << bestRoute.cost << std::endl;
            std::cout << "Time taken: " << duration << " ms" << std::endl;
            std::cout << "--------------------------------------------------------" << std::endl;
        }
    }

    MPI_Finalize();
}
