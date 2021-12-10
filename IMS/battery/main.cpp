/**
 *  IMS - project
 */
#include <simlib.h>

#define MINUTE 1
#define HOUR 60 * MINUTE
#define DAY 24 * HOUR
#define YEAR 365 * DAY
#define DISMANTLING_CENTRE 9
#define TRUCK 5
#define PLANT 12000

#define EMISSION 31.2

#define PLANT_EMISSION 700

int batteries_sum;
int number_of_batteries_waiting_for_truck;
int battery_pack;
int batteries_on_plant;
int batteries_processed;
double co2_output;

Facility Notification("Notification");

Store DismantlingCentre("Centre", DISMANTLING_CENTRE);
Store Truck("Truck", TRUCK);
Store Plant("Plant", PLANT);

class PlantProcessing: public Process{
    void Behavior() {
        Enter(Plant, 1);
        batteries_on_plant --;
        Wait(Uniform(8 * HOUR, 12 * HOUR));
        co2_output -= PLANT_EMISSION;
        batteries_processed++;
        Leave(Plant, 1);
    }
};

class Pack : public Process{
    void Behavior() {
        Enter(Truck, 1);
        // Trip from plant to dismantling centre
        double start_trip = Time;
        Wait(Uniform(0.5 * HOUR, 3.5 * HOUR));
        double end_trip = Time;
        co2_output += (end_trip - start_trip) * EMISSION;

        // Batteries loading
        Wait(Exponential(1 * HOUR));
        battery_pack--;

        // Trip from dismantling centre to the plant
        start_trip = Time;
        Wait(Uniform(0.75 * HOUR, 5.25 * HOUR));
        end_trip = Time;
        co2_output += (end_trip - start_trip) * EMISSION;

        // Batteries unloading
        for (int i = 0; i < 6; i++) {
            Wait(Exponential(15 * MINUTE));
            batteries_on_plant ++;
        }

        Leave(Truck, 1);
        (new PlantProcessing)->Activate();
    }
};

class Auto : public Process {
    void Behavior() {
        // Battery End-of-life 
        Wait(Exponential(10*YEAR));
        Enter(DismantlingCentre, 1);
        Wait(2 * HOUR);
        Leave(DismantlingCentre, 1);
        number_of_batteries_waiting_for_truck++;
        batteries_sum++;

        if (number_of_batteries_waiting_for_truck == 6) {
            Seize(Notification);
            number_of_batteries_waiting_for_truck -= 6;
            battery_pack++;
            Release(Notification);
            // Print("battery_pack: %d\n", battery_pack); 
            (new Pack)->Activate();
        }       
    }
};

class Generator : public Event {
    void Behavior() {
        (new Auto)->Activate();
        Activate(Time + Exponential(0.5 * DAY));
    }
};

int main(int argc, char*argv[]) {
    int simtime = 10;
    batteries_sum = 0;
    number_of_batteries_waiting_for_truck = 0;
    battery_pack = 0;
    co2_output = 0;
    batteries_on_plant = 0;
    batteries_processed = 0;

    Init(0, 25 * YEAR); // doba simulace
    (new Generator)->Activate();
    Run(); // start simulace

    Print("Batteries: %d\n", batteries_sum);
    Print("CO2 output: %f\n", co2_output);
    Print("number_of_batteries_waiting_for_truck: %d\n", number_of_batteries_waiting_for_truck);
    Print("battery_pack: %d\n", battery_pack);
    Print("batteries_on_plant: %d\n", batteries_on_plant);
    Print("Batteries_processed: %d\n", batteries_processed);
    Print("Total: %d\n", number_of_batteries_waiting_for_truck + batteries_on_plant + batteries_processed);
    return EXIT_SUCCESS;
}