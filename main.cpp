#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>

using namespace std;

const int RED_DURATION = 5; // Seconds
const int GREEN_DURATION = 4; // Seconds
const int YELLOW_DURATION = 2; // Seconds

mutex mtx;
condition_variable cv;
bool stopSimulation = false;
vector<string> trafficLightStates(4, "Red"); // Initial state of all lights is Red
vector<pair<int, int>> trafficLightPriority; // Pair of (lightID, carCount)
int currentActiveLightIndex = 0; // Index in the sorted priority order
bool isTwoLightsGreen = false; // Mode flag
int cycleCount = 0; // To track completed cycles

// Function to display the state of all traffic lights with directions
void displayTrafficLightStates() {
vector<string> directions = {"North", "West", "South", "East"};
for (int i = 0; i < 4; ++i) {
cout << "Traffic Light " << i + 1 << " (" << directions[i] << "): " << trafficLightStates[i] << "\n";
}
cout << endl;
}

// Traffic light simulation function
void trafficLightSimulation(int threadIndex) {
while (true) {
unique_lock<mutex> lock(mtx);
cv.wait(lock, [threadIndex]() { return currentActiveLightIndex == threadIndex || stopSimulation; });

if (stopSimulation) break;

int lightID = trafficLightPriority[threadIndex].first;

// Set the current light(s) to Green
if (isTwoLightsGreen) {
trafficLightStates[lightID] = "Green";
trafficLightStates[(lightID + 2) % 4] = "Green"; // Opposite light in the pair
} else {
trafficLightStates[lightID] = "Green";
}
displayTrafficLightStates();
this_thread::sleep_for(chrono::seconds(GREEN_DURATION));

// Set the current light(s) to Yellow
if (isTwoLightsGreen) {
trafficLightStates[lightID] = "Yellow";
trafficLightStates[(lightID + 2) % 4] = "Yellow"; // Opposite light in the pair
} else {
trafficLightStates[lightID] = "Yellow";
}
displayTrafficLightStates();
this_thread::sleep_for(chrono::seconds(YELLOW_DURATION));

// Set the current light(s) to Red and move to the next light or pair in priority order
if (isTwoLightsGreen) {
trafficLightStates[lightID] = "Red";
trafficLightStates[(lightID + 2) % 4] = "Red";
currentActiveLightIndex = (currentActiveLightIndex + 1) % 2; // Cycle between two pairs (0 and 1)
} else {
trafficLightStates[lightID] = "Red";
currentActiveLightIndex = (currentActiveLightIndex + 1) % 4;
}

// Increment cycle count when returning to the first light
if (currentActiveLightIndex == 0) {
cycleCount++;
if (cycleCount == 1) {
stopSimulation = true;
}
}

// Notify all threads to update their state
cv.notify_all();
}
}

int main() {
cout << "===================================================\n";
cout << "             Traffic Light Simulation\n";
cout << "===================================================\n";

// Edge case handling: Green mode input validation
int greenMode;
cout << "How many traffic lights will be green (1 or 2): ";
while (!(cin >> greenMode) || (greenMode != 1 && greenMode != 2)) {
cout << "Invalid input. Enter 1 or 2: ";
cin.clear();
cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
isTwoLightsGreen = (greenMode == 2);

// Edge case handling: Car counts input validation
cout << "Enter the number of cars for each traffic light \n";
for (int i = 0; i < 4; ++i) {
int cars;
cout << "Traffic Light " << i + 1 << ": ";
while (!(cin >> cars) || cars < 0) {
cout << "Invalid input. Enter a non-negative number: ";
cin.clear();
cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
trafficLightPriority.push_back({i, cars});
}

// Edge case: Handle all zero car counts
if (all_of(trafficLightPriority.begin(), trafficLightPriority.end(), [](pair<int, int> p) { return p.second == 0; })) {
cout << "No cars at any traffic light. Simulation will not run.\n";
return 0;
}

// Sort traffic lights by descending car count
if (isTwoLightsGreen) {
vector<int> pairCounts = {
trafficLightPriority[0].second + trafficLightPriority[2].second,
trafficLightPriority[1].second + trafficLightPriority[3].second
};

// Determine pair priority (0 = North-South, 1 = West-East)
if (pairCounts[1] > pairCounts[0]) {
swap(trafficLightPriority[0], trafficLightPriority[1]);
swap(trafficLightPriority[2], trafficLightPriority[3]);
}
} else {
sort(trafficLightPriority.begin(), trafficLightPriority.end(),
[](const pair<int, int>& a, const pair<int, int>& b) {
if (a.second == b.second) return a.first < b.first; // Tie-breaking by lightID
return a.second > b.second;
});
}

cout << "\nStarting traffic light simulation...\n";

vector<thread> trafficLights;

// Create threads for traffic lights based on mode
int threadCount = isTwoLightsGreen ? 2 : 4;
for (int i = 0; i < threadCount; ++i) {
trafficLights.push_back(thread(trafficLightSimulation, i));
}

// Wait for all threads to finish
for (auto& t : trafficLights) {
t.join();
}

cout << "Simulation stopped. All traffic lights are red.\n";

return 0;
}