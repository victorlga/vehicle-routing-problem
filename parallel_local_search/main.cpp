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
#include <mpi.h>

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
    int world_rank;
    int world_size;

    CapacitatedVehicleRoutingProblem(
        int numberOfPlaces,
        int vehicleCapacity,
        int maxNumberOfPlacesPerRoute,
        std::map<Place, Load> placesDemand,
        std::map<Place, std::map<Place, Cost>> roads,
        int world_rank,
        int world_size) : numberOfPlaces(numberOfPlaces), vehicleCapacity(vehicleCapacity),
                          maxNumberOfPlacesPerRoute(maxNumberOfPlacesPerRoute), placesDemand(placesDemand),
                          roads(roads), world_rank(world_rank), world_size(world_size) {}

    void solve()
    {
        int iterations = 10000;                                                           // Total number of iterations
        int local_iterations = iterations / world_size;                                   // Divide iterations among MPI processes
        int start = local_iterations * world_rank;                                        // Starting index for each process
        int end = (world_rank == world_size - 1) ? iterations : start + local_iterations; // End index, adjust for the last process

        Route localBestRoute;
        Cost localLowerCost = INT_MAX;

        #pragma omp parallel
        {
            Route threadBestRoute;
            Cost threadLowerCost = INT_MAX;

            #pragma omp for nowait
            for (int i = start; i < end; ++i)
            {
                std::pair<Route, Cost> result = generateRouteAndCost();
                if (result.second < threadLowerCost)
                {
                    threadBestRoute = result.first;
                    threadLowerCost = result.second;
                }
            }

            #pragma omp critical
            {
                if (threadLowerCost < localLowerCost)
                {
                    localBestRoute = threadBestRoute;
                    localLowerCost = threadLowerCost;
                }
            }
        }

        // Use MPI_Reduce to find the global best route and cost
        struct {
            Cost cost;
            int rank;
        } localResult = {localLowerCost, world_rank}, globalResult;

        MPI_Reduce(&localResult, &globalResult, 1, MPI_2INT, MPI_MINLOC, 0, MPI_COMM_WORLD);

        if (world_rank == 0)
        {
            if (globalResult.rank == 0) {
                bestRoute = localBestRoute;
            } else {
                MPI_Recv(bestRoute.data(), bestRoute.size(), MPI_INT, globalResult.rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        } else {
            if (world_rank == globalResult.rank) {
                MPI_Send(localBestRoute.data(), localBestRoute.size(), MPI_INT, 0, 0, MPI_COMM_WORLD);
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

    std::pair<Place, Cost> findCheaperValidRoad(std::map<Place, Cost> availableRoads, int &numberOfPlacesVisited, Load &vehicleLoad, std::set<Place> &placesVisited, Place previousPlace)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> uniformRealDistr(0.0, 1.0);
        std::uniform_int_distribution<> uniformIntDistr(0, INT_MAX);

        std::pair<Place, Cost> cheaperRoad(INT_MAX, INT_MAX);

        for (auto const &road : availableRoads)
        {
            bool newPlaceOrStart = (placesVisited.find(road.first) == placesVisited.end() || road.first == 0);
            if (road.second < cheaperRoad.second && newPlaceOrStart && road.first != previousPlace)
                cheaperRoad = road;
        }

        if (uniformRealDistr(gen) > 0.5)
        {
            std::vector<Place> availablePlaces;
            for (const auto &road : availableRoads)
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

int main(int argc, char* argv[])
{
    std::vector<std::string> fileNames = {
        "../graphs/graph0.txt",
        "../graphs/graph1.txt",
        "../graphs/graph2.txt",
        "../graphs/graph3.txt"};

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

        Load vehicleCapacity = 10;
        int maxNumberOfPlacesPerRoute = 4;

        MPI_Init(&argc, &argv);

        int world_size, world_rank;
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

        CapacitatedVehicleRoutingProblem CVRP = CapacitatedVehicleRoutingProblem(
            numberOfPlaces,
            vehicleCapacity,
            maxNumberOfPlacesPerRoute,
            placesDemand,
            roads,
            world_rank,
            world_size
        );

        CVRP.solve();

        MPI_Finalize();

        Route bestRoute = CVRP.bestRoute;
        Cost lowerCost = CVRP.lowerCost;

        std::cout << "Running solution for " << fileNames[j] << std::endl;
        std::cout << "Best route Place sequence: ";
        for (Place &place : bestRoute)
            std::cout << place << " -> ";
        std::cout << std::endl
                  << "Best route cost: " << lowerCost << std::endl;
        std::cout << "--------------------------------------------------------" << std::endl;
    }
}