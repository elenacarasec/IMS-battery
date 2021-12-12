/**
 *  IMS - project
 */
#include <simlib.h>
#include <getopt.h>
#include <iostream>

#define MINUTE 1
#define HOUR 60 * MINUTE
#define DAY 24 * HOUR
#define YEAR 365 * DAY
#define DISMANTLING_CENTRE 14
#define TRUCK 1
#define PLANT 3

#define EMISSION 0.03122

#define PLANT_EMISSION 736

int car;
int batteries_dead;
int batteries_sum;
int number_of_batteries_waiting_for_truck;
int battery_pack;
int batteries_in_trip;
int batteries_on_plant;
int batteries_processed;
double co2_truck;
double co2_output;
bool is_day;

Facility Notification("Notification");

Store DismantlingCentre("Centre", DISMANTLING_CENTRE);
Store Truck("Truck", TRUCK);
Store Plant("Plant", PLANT);

//statistiky
Histogram trip_to_centre("Trip to centre",  30, 10 * MINUTE, 18);
Histogram notification("Notification",  0, HOUR, 24);
Histogram dism("Dismantling centre",  0, HOUR, 24);
Histogram car_hist("Car Lifecycle",  0, YEAR, 20);
// Histogram co2_hist("CO2 output",  0, YEAR, 20);

Stat dead_bat;
Stat bat_on_plant;
TStat co2_hist;

class PlantProcessing: public Process{
    void Behavior() {
        Enter(Plant, 1);
        Wait(Uniform(8 * HOUR, 12 * HOUR));
        batteries_on_plant--;
        batteries_processed++;
        co2_output -= PLANT_EMISSION;
        Leave(Plant, 1);
    }
};

class Pack : public Process{
    void Behavior() {
        Enter(Truck, 1);
        battery_pack--;
        batteries_in_trip += 6;

        double start_trip = Time;
        Wait(Uniform(0.5 * HOUR, 3.5 * HOUR));
        double end_trip = Time;
        co2_output += (end_trip - start_trip) / 60 * EMISSION;
        co2_truck += (end_trip - start_trip) / 60 * EMISSION;
        trip_to_centre(end_trip - start_trip);

        // Batteries loading
        Wait(Exponential(1 * HOUR));  

        // Trip from dismantling centre to the plant
        start_trip = Time;
        Wait(Uniform(0.75 * HOUR, 5.25 * HOUR));
        end_trip = Time;
        co2_output += (end_trip - start_trip) / 60 * EMISSION;
        co2_truck += (end_trip - start_trip) / 60 * EMISSION;

        co2_hist(co2_output);

        batteries_in_trip-=6;
        // Batteries unloading
        for (int i = 0; i < 6; i++) {
            Wait(Exponential(15 * MINUTE));
            batteries_on_plant ++;
            bat_on_plant(batteries_on_plant);
            (new PlantProcessing)->Activate();
        }

        Leave(Truck, 1);
    }
};

class DismCentre : public Process {
    void Behavior() {
        if (is_day && batteries_dead > 0) {
            batteries_dead--;
            Enter(DismantlingCentre, 1);
            Wait(2 * HOUR);
            Leave(DismantlingCentre, 1);

            number_of_batteries_waiting_for_truck++;
            if (number_of_batteries_waiting_for_truck == 6) {
                double start_notif = Time;
                Seize(Notification);
                number_of_batteries_waiting_for_truck -= 6;
                battery_pack++;
                
                while (!is_day) {
                    Wait(8 * HOUR + 1 * MINUTE);
                }
                (new Pack)->Activate();

                Release(Notification);
                notification(Time - start_notif);
            }
        }
    }
};


class Auto : public Process {
    void Behavior() {
        // Battery End-of-life
        car++;
        double car_life = Time;
        Wait(Uniform(7 * YEAR, 10 * YEAR));
        batteries_sum++;
        batteries_dead++;
        dead_bat(batteries_dead);
        car_hist(Time - car_life);

        double start_dism = Time;
        while (!is_day)
           Wait(8 * HOUR + 1 * MINUTE);

        (new DismCentre)->Activate();
        dism(Time - start_dism);
    }
};

class Workshift : public Event {
	void Behavior() {
		if(!is_day) {
			is_day = true;
			Activate(Time + (8 * HOUR));
		} else {
			is_day = false;
			Activate(Time + (16 * HOUR));
		}
	}
};

class Generator : public Event {
    void Behavior() {
        (new Auto)->Activate();
        Activate(Time + Exponential(0.25 * DAY));
    }
};

void Help()
{
	std::cerr << "Possible arguments:\n"
		<< "\t-t or --cars to specify the number of cars\n"
		<< "\t-f or --food to specify the number of food\n";
}

int main(int argc, char*argv[]) {

    int opt;
	char *err;
	static const char *sOpts = "t:c:p:";
	static const struct option lOpts[] = {
		{"time", required_argument, nullptr, 't'},
		{"cars", required_argument, nullptr, 'c'},
		{"plant", required_argument, nullptr, 'p'},
		{nullptr, 0, nullptr, 0},
	};

    // 	while ((opt = getopt_long(argc, argv, sOpts, lOpts, nullptr)) != -1)
	// {
	// 	switch (opt)
	// 	{
	// 		case 0:
	// 			break;

	// 		case 'c':
	// 			cars = strtoul(optarg, &err, 10);

	// 			if (*err != '\0')
	// 			{
	// 				cerr << "Number of cars must be a positive integer.\n";
	// 				return EXIT_FAILURE;
	// 			}

	// 			break;

	// 		case 'f':
	// 			food = strtod(optarg, &err);

	// 			if (*err != '\0')
	// 			{
	// 				cerr << "Number of food must be a positive number.\n";
	// 				return EXIT_FAILURE;
	// 			}

	// 			if (food < 0)
	// 			{
	// 				cerr << "Number of food must be a positive number.\n";
	// 				return EXIT_FAILURE;
	// 			}

	// 			break;

	// 		case '?':
	// 		default:
	// 			PrintHelp();
	// 			return EXIT_FAILURE;
	// 	}
	// }

    is_day = false;
    car = 0;
    batteries_sum = 0;
    number_of_batteries_waiting_for_truck = 0;
    battery_pack = 0;
    co2_truck = 0;
    co2_output = 0;
    batteries_in_trip = 0;
    batteries_on_plant = 0;
    batteries_processed = 0;

    RandomSeed(time(NULL));
    Init(0, 25 * YEAR); // doba simulace

    (new Workshift)->Activate();
    (new Generator)->Activate();
    Run(); // start simulace

    car_hist.Output();
    // dead_bat.Output();
    // bat_on_plant.Output();
    co2_hist.Output();


    Print("Car: %d\n", car);
    Print("Dead batteries %d\n", batteries_sum);
    Print("Batteries dead: %d\n", batteries_dead);
    Print("number_of_batteries_waiting_for_truck: %d\n", number_of_batteries_waiting_for_truck);
    Print("battery_pack: %d\n", battery_pack * 6);
    Print("batteries_in_trip: %d\n", batteries_in_trip);
    Print("batteries_on_plant: %d\n", batteries_on_plant);
    Print("Batteries_processed: %d\n", batteries_processed);
    Print("Total: %d\n", number_of_batteries_waiting_for_truck + battery_pack*6 + batteries_in_trip + batteries_on_plant + batteries_processed);
    Print("CO2 truck: %.2ft\n", co2_truck / 1000);
    Print("CO2 output: %.2ft\n", co2_output / 1000);
    
    return EXIT_SUCCESS;
}