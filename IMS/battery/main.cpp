/**
 *  IMS - project "Battery recycling"
 *  Authors: Elena Carasec (xcaras00),
 *           Viktoryia Tomason (xtomas34)
 */

#include <simlib.h>
#include <getopt.h>
#include <iostream>

// time units
#define MINUTE 1
#define HOUR 60 * MINUTE
#define DAY 24 * HOUR
#define YEAR 365 * DAY

// dismantling centre capacity
#define DISMANTLING_CENTRE 14
// available trucks
#define TRUCK 1
// plant lines
#define PLANT 2

// truck emission of C02 (kg) per km
#define EMISSION 0.03122
// plant emission savings of C02 (kg) per battery kWh
#define PLANT_EMISSION 736

// For debug
// Sold cars
int car = 0;
// Batteries in their end-of-life period
int batteries_dead = 0;
// Batteries waiting for dismantle
int batteries_sum = 0;
// Batteries before truck driver notification
int number_of_batteries_waiting_for_truck = 0;
// Batteries after truck driver notification
int battery_pack = 0;
// Batteries on their way to plant
int batteries_in_trip = 0;
// Batteries before processing
int batteries_on_plant = 0;
// Batteries afteer processing
int batteries_processed = 0;

// Truck CO2 output in kg
double co2_truck = 0;
// Plant CO2 output in kg
double co2_output = 0;
// Workshift period
bool is_day = false;

/**
 * @brief Strucure with input arguments.
 */
struct input_args {
    unsigned int time; // simulation time
    unsigned int cars; // sold cars per year
    unsigned int plant; // number of plant lines
} input_args;

// Call truck driver when there are 6 batteries to carry
Facility Notification("Notification");

// Dismantling centre
Store DismantlingCentre("Centre", DISMANTLING_CENTRE);
// Available trucks
Store Truck("Truck", TRUCK);
// Recy Plant
Store Plant("Plant", PLANT);

//statistiky
Histogram car_hist("Car Lifecycle",  7 * YEAR, YEAR, 3 );
Histogram car_gen("Car output",  0, YEAR, 20);


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

        // Batteries loading
        Wait(Exponential(1 * HOUR));  

        // Trip from dismantling centre to the plant
        start_trip = Time;
        Wait(Uniform(0.75 * HOUR, 5.25 * HOUR));
        end_trip = Time;
        co2_output += (end_trip - start_trip) / 60 * EMISSION;
        co2_truck += (end_trip - start_trip) / 60 * EMISSION;

        batteries_in_trip-=6;
        // Batteries unloading
        for (int i = 0; i < 6; i++) {
            Wait(Exponential(15 * MINUTE));
            batteries_on_plant ++;
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
                    Wait(8 * HOUR);
                }
                (new Pack)->Activate();

                Release(Notification);
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
        car_hist(Time - car_life);

        double start_dism = Time;
        while (!is_day)
           Wait(8 * HOUR);

        (new DismCentre)->Activate();
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
        Activate(Time + Exponential(YEAR / input_args.cars));
        car_gen(Time);
    }
};

void print_help()
{
	std::cerr << "Possible arguments:\n"
        << "\t-h or --help : This help message\n"
		<< "\t-t or --time : Simulation time in years\n"
		<< "\t-c or --cars : Number of cars sold per year\n"
		<< "\t-p or --plant : Number of lines in the plant\n";
}

int parse_arguments(int argc, char **argv) {
    int opt;
	char *err;
	static const char *short_opts = "t:c:p:";
	static const struct option long_opts[] = {
        {"help", required_argument, nullptr, 'h'},
		{"time", required_argument, nullptr, 't'},
		{"cars", required_argument, nullptr, 'c'},
		{"plant", required_argument, nullptr, 'p'},
		{nullptr, 0, nullptr, 0},
	};

    input_args.cars = 1;
    input_args.time = 1;
    input_args.plant = 1;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
		switch (opt) {
			case 0:
				break;

			case 't':
				input_args.time = strtoul(optarg, &err, 10);
				if (*err != '\0' || input_args.time <= 0) {
					std::cerr << "Simulation time must be a positive integer greater than 0.\n";
					return EXIT_FAILURE;
				}
				break;

			case 'c':
				input_args.cars = strtoul(optarg, &err, 10);
				if (*err != '\0' || input_args.cars <= 0) {
					std::cerr << "Number of sold cars must be a positive integer greater than 0.\n";
					return EXIT_FAILURE;
				}
				break;
            
            case 'p':
				input_args.plant = strtoul(optarg, &err, 10);
				if (*err != '\0' || input_args.plant < 0) {
					std::cerr << "Number of plant lines must be a positive integer greater than 0.\n";
					return EXIT_FAILURE;
				}
				break;

            case 'h':
                print_help();
                return EXIT_SUCCESS;

			case '?':
			default:
				print_help();
				return EXIT_FAILURE;
		}
	}
    return EXIT_SUCCESS;
}

int main(int argc, char*argv[]) {
    int ret = parse_arguments(argc, argv);
    if (ret != 0) {
        return ret;
    }

    // is_day = false;
    // car = 0;
    // batteries_sum = 0;
    // number_of_batteries_waiting_for_truck = 0;
    // battery_pack = 0;
    // co2_truck = 0;
    // co2_output = 0;
    // batteries_in_trip = 0;
    // batteries_on_plant = 0;
    // batteries_processed = 0;

    RandomSeed(time(NULL));
    Init(0, input_args.time * YEAR); // simulation time

    (new Workshift)->Activate();
    (new Generator)->Activate();
    Run(); // simulation start

    Print("Sold cars: %d\n", car);
    Print("Bateries end-of-life: %d\n", batteries_sum);
    Print("Batteries in dismantling centre: %d\n", batteries_dead);
    Print("Batteries waiting for truck driver notification: %d\n", number_of_batteries_waiting_for_truck);
    Print("Battery pack: %d\n", battery_pack * 6);
    Print("Batteries in trip: %d\n", batteries_in_trip);
    Print("Batteries on plant: %d\n", batteries_on_plant);
    Print("Batteries processed: %d\n", batteries_processed);
    Print("CO2 truck: %.2fkg\n", co2_truck);
    Print("CO2 output: %.2ft\n", co2_output / 1000);

    car_hist.Output();
    car_gen.Output();

    return EXIT_SUCCESS;
}