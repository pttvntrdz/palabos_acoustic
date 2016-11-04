#include "palabos3D.h"
#include "palabos3D.hh"
#include <vector>
#include <cmath>
#include <time.h>
#include <cfloat> 

using namespace plb;
using namespace plb::descriptors;
using namespace std;


typedef double T;
typedef Array<T,3> Velocity;
#define DESCRIPTOR MRTD3Q19Descriptor
 typedef MRTdynamics<T,DESCRIPTOR> BackgroundDynamics;
 typedef AnechoicMRTdynamics<T,DESCRIPTOR> AnechoicBackgroundDynamics;

// ---------------------------------------------
// Includes of acoustics resources
#include "acoustics/acoustics3D.h"
using namespace plb_acoustics_3D;
// ---------------------------------------------

const T rho0 = 1;
const T drho = rho0/100;

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void writeGifs(MultiBlockLattice3D<T,DESCRIPTOR>& lattice, plint iter){
    const plint nx = lattice.getNx();
    const plint ny = lattice.getNy();
    const plint nz = lattice.getNz();

    const plint imSize = 600;
    ImageWriter<T> imageWriter("leeloo");
    
    Box3D slice(0, nx-1, 0, ny-1, nz/2, nz/2);
    //imageWriter.writeGif(createFileName("u", iT, 6),
     //*computeDensity(lattice), );

    imageWriter.writeGif( createFileName("rho", iter, 6),
    *computeDensity(lattice, slice), 
    (T) rho0 - drho/1000000, (T) rho0 + drho/1000000, imSize, imSize);
}

void writeVTK(MultiBlockLattice3D<T,DESCRIPTOR>& lattice, plint iter){
        VtkImageOutput3D<T> vtkOut(createFileName("vtk", iter, 6), 1.);
        vtkOut.writeData<float>(*computeDensity(lattice), "density", 1.);
        std::auto_ptr<MultiScalarField3D<T> > velocity(plb::computeVelocityComponent(lattice, lattice.getBoundingBox(), 2));
        
        vtkOut.writeData<T>(*velocity, "velocity", 1.);
}

void build_duct(MultiBlockLattice3D<T,DESCRIPTOR>& lattice, plint nx, plint ny,
    Array<plint,3> position, plint radius, plint length, plint thickness, T omega){
    length += 4;
    plint anechoic_size = 20;
    // Duct is constructed along the Z direction
    //plint size_square = 50;
    plint size_square = 2*radius;
    plint radius_intern = radius - thickness;
    for (plint x = position[0] - radius; x < nx/2 + size_square/2; ++x){
        for (plint y = position[1] - radius; y < ny/2 + size_square/2; ++y){
            for (plint z = position[2]; z < length + position[2]; ++z){

                if (radius*radius > (x-nx/2)*(x-nx/2) + (y-ny/2)*(y-ny/2)){
                    DotList3D points_to_aplly_dynamics;
                    points_to_aplly_dynamics.addDot(Dot3D(x,y,z));
                    defineDynamics(lattice, points_to_aplly_dynamics, new BounceBack<T,DESCRIPTOR>(0));
                }
                // extrude
                if (radius_intern*radius_intern > (x-nx/2)*(x-nx/2) + (y-ny/2)*(y-ny/2) && z > position[2] + 2){
                    DotList3D points_to_aplly_dynamics;
                    points_to_aplly_dynamics.addDot(Dot3D(x,y,z));
                    if (z < position[2] + 2 + anechoic_size && z > position[2] + 3){
                        Array<T,3> u0(0, 0, 0);
                        T rhoBar_target = 0;
                        AnechoicBackgroundDynamics *anechoicDynamics = 
                        new AnechoicBackgroundDynamics(omega);
                        T delta_efective = anechoic_size - z - position[2] - 2;
                        anechoicDynamics->setDelta(delta_efective);
                        anechoicDynamics->setRhoBar_target(rhoBar_target);
                        anechoicDynamics->setJ_target(u0);
                        defineDynamics(lattice, points_to_aplly_dynamics, anechoicDynamics);
                        
                    }else{
                        defineDynamics(lattice, points_to_aplly_dynamics, new BackgroundDynamics(omega));
                    }
                }
            }
        }
    }
}

T get_linear_chirp(T ka_min, T ka_max, plint maxT_final_source, plint iT, T drho, T radius){
    T lattice_speed_sound = 1/sqrt(3);
    T initial_frequency = ka_min*lattice_speed_sound/(2*M_PI*radius);
    T frequency_max_lattice = ka_max*lattice_speed_sound/(2*M_PI*radius);
    T variation_frequency = (frequency_max_lattice - initial_frequency)/maxT_final_source;
    T frequency_function = initial_frequency*iT + (variation_frequency*iT*iT)/2;
    T phase = 2*M_PI*frequency_function;
    T chirp_hand = 1. + drho*sin(phase);

    return chirp_hand;
}

T get_linear_chirp_AZ(T ka_max, plint total_signals, plint maxT_final_source, plint iT, T drho, T radius){
    T cs = 1/sqrt(3);
    T chirp_value = 1;
    total_signals = 2*total_signals;
    for (plint n_signal = 1; n_signal <= total_signals; n_signal++){
        T interval = ka_max/total_signals;
        T phase = (n_signal*interval*cs*iT)/(radius);
        chirp_value += drho*sin(phase);
    }
    return chirp_value;
}


void set_source(MultiBlockLattice3D<T,DESCRIPTOR>& lattice, Array<plint,3> position, 
    T chirp_hand, Array<T,3> u0, plint radius, plint radius_intern, plint nx, plint ny){

    plint size_square = 2*radius;
    Box3D impulse_local;
    for (plint x = position[0] - radius; x < nx/2 + size_square/2; ++x){
        for (plint y = position[1] - radius; y < ny/2 + size_square/2; ++y){
            // extrude
            if (radius_intern*radius_intern > (x-nx/2)*(x-nx/2) + (y-ny/2)*(y-ny/2)){
                Array<plint, 6> local_source_1(x, x, y, y, position[2] + 2 + 20, position[2] + 2 + 20);
                impulse_local.from_plbArray(local_source_1);
                initializeAtEquilibrium(lattice, impulse_local, chirp_hand, u0);
                Array<plint, 6> local_source_2(x, x, y, y, position[2] + 2 + 20, position[2] + 2 + 21);
                impulse_local.from_plbArray(local_source_2);
                initializeAtEquilibrium(lattice, impulse_local, chirp_hand, u0);
            }
        }
    }
}

T compute_avarage_density(MultiBlockLattice3D<T,DESCRIPTOR>& lattice, Array<plint,3> position, 
    plint radius, plint radius_intern, plint nx, plint ny, plint position_z){

    plint number_cells = 0;
    T average_density = 0;
    plint size_square = 2*radius;
    Box3D local_box;
    for (plint x = position[0] - radius; x < nx/2 + size_square/2; ++x){
        for (plint y = position[1] - radius; y < ny/2 + size_square/2; ++y){
            // extrude
            if (radius_intern*radius_intern > (x-nx/2)*(x-nx/2) + (y-ny/2)*(y-ny/2)){
                Array<plint, 6> local(x, x, y, y, position_z, position_z);
                local_box.from_plbArray(local);
                average_density += computeAverageDensity(lattice, local_box);
                number_cells += 1;
            }
        }
    }

    average_density = average_density/number_cells;

    return average_density;
}

T compute_avarage_velocity(MultiBlockLattice3D<T,DESCRIPTOR>& lattice, Array<plint,3> position, 
    plint radius, plint radius_intern, plint nx, plint ny, plint position_z){

    plint number_cells = 0;
    T average_velocity = 0;
    plint size_square = 2*radius;
    Box3D local_box;
    for (plint x = position[0] - radius; x < nx/2 + size_square/2; ++x){
        for (plint y = position[1] - radius; y < ny/2 + size_square/2; ++y){
            // extrude
            if (radius_intern*radius_intern > (x-nx/2)*(x-nx/2) + (y-ny/2)*(y-ny/2)){
                Array<plint, 6> local(x, x, y, y, position_z, position_z);
                local_box.from_plbArray(local);
                std::auto_ptr<MultiScalarField3D<T> > velocity(plb::computeVelocityComponent(lattice, local_box, 2));
                average_velocity += computeAverage(*velocity, local_box);
                number_cells += 1;
            }
        }
    }

    average_velocity = average_velocity/number_cells;

    return average_velocity;
}


void set_nodynamics(MultiBlockLattice3D<T,DESCRIPTOR>& lattice, plint nx, plint ny, plint nz, plint diameter){
    Box3D place_nodynamics(0, nx - 1, 0, ny - 1, 0, 30 + 3*diameter - 1);
    defineDynamics(lattice, place_nodynamics, new NoDynamics<T,DESCRIPTOR>(0));
}

class Probe{
    private:
        Box3D location;
        string name_probe;
        plb_ofstream file_pressures;
        plb_ofstream file_velocities_x;
        plb_ofstream file_velocities_y;
        plb_ofstream file_velocities_z;
    public:
        Probe(Box3D location, string directory, string name_probe){

            directory = directory + "/" + name_probe;
            std::string command = "mkdir -p " + directory;
            char to_char_command[1024];
            strcpy(to_char_command, command.c_str());
            system(to_char_command);

            string pressures_string = directory + "/history_pressures_" + name_probe + ".dat";
            string velocities_x_string = directory + "/history_velocities_x_" + name_probe + ".dat";
            string velocities_y_string = directory + "/history_velocities_y_" + name_probe + ".dat";
            string velocities_z_string = directory + "/history_velocities_z_" + name_probe + ".dat";
            char to_char_pressures[1024];
            char to_char_velocities_x[1024];
            char to_char_velocities_y[1024];
            char to_char_velocities_z[1024];
            strcpy(to_char_pressures, pressures_string.c_str());
            strcpy(to_char_velocities_x, velocities_x_string.c_str());
            strcpy(to_char_velocities_y, velocities_y_string.c_str());
            strcpy(to_char_velocities_z, velocities_z_string.c_str());

            this->file_pressures.open(to_char_pressures);
            this->file_velocities_x.open(to_char_velocities_x);
            this->file_velocities_y.open(to_char_velocities_y);
            this->file_velocities_z.open(to_char_velocities_z);
            this->location = location;
            this->name_probe = name_probe;
        }

        string get_name_probe(){
            return this->name_probe;
        }

        void save_point(MultiBlockLattice3D<T,DESCRIPTOR>& lattice, T rho0, T cs2){
            file_pressures << setprecision(10) << (computeAverageDensity(lattice, this->location) - rho0)*cs2 << endl;
            std::auto_ptr<MultiScalarField3D<T> > velocity_x(plb::computeVelocityComponent(lattice, this->location, 0));
            file_velocities_x << setprecision(10) << computeAverage(*velocity_x, this->location) << endl;
            std::auto_ptr<MultiScalarField3D<T> > velocity_y(plb::computeVelocityComponent(lattice, this->location, 1));
            file_velocities_y << setprecision(10) << computeAverage(*velocity_y, this->location) << endl;
            std::auto_ptr<MultiScalarField3D<T> > velocity_z(plb::computeVelocityComponent(lattice, this->location, 2));
            file_velocities_z << setprecision(10) << computeAverage(*velocity_z, this->location) << endl;
        }
};

int main(int argc, char **argv){
    plbInit(&argc, &argv);

    //const plint length_domain = 420;
    const plint radius = 10;
    const plint diameter = 2*radius;
    //const plint length_domain = 150;
    const plint nx = 6*diameter + 60;
    const plint ny = 6*diameter + 60;
    const plint position_duct_z = 0;
    const plint nz = 9*diameter + 60;
    const T lattice_speed_sound = 1/sqrt(3);
    const T omega = 1.985;
    const plint maxT = pow(2,13) + nz*sqrt(3);
    const Array<T,3> u0(0, 0, 0);
    const Array<plint,3> position(nx/2, ny/2, position_duct_z);
    const plint length_duct = 6*diameter + 30;
    const plint thickness_duct = 2;
    const plint radius_intern = radius - 2;
    const plint boca_duct = 30 + 6*diameter - 1;
    //const plint maxT = 2*120/lattice_speed_sound;
    const plint maxT_final_source = maxT - nz*sqrt(3);
    const T ka_max = 2.5;
    //const T ka_min = 0;
    const T cs2 = lattice_speed_sound*lattice_speed_sound;
    const plint source_radius = radius - 1;
    clock_t t;

    // Saving a propery directory
    std::string fNameOut = currentDateTime() + "+tmp";
    std::string command = "mkdir -p " + fNameOut;
    char to_char_command[1024];
    strcpy(to_char_command, command.c_str());
    system(to_char_command);
    std::string command_copy_script_matlab = "cp duct_radiation.m " + fNameOut;
    char to_char_command_copy_script_matlab[1024];
    strcpy(to_char_command_copy_script_matlab, command_copy_script_matlab.c_str());
    system(to_char_command_copy_script_matlab);
    global::directories().setOutputDir(fNameOut+"/");


    // Setting anechoic dynamics like this way
    MultiBlockLattice3D<T, DESCRIPTOR> lattice(nx, ny, nz,  new AnechoicBackgroundDynamics(omega));
    defineDynamics(lattice, lattice.getBoundingBox(), new BackgroundDynamics(omega));

    pcout << "Creation of the lattice. Time max: " << maxT << endl;
    pcout << "Duct radius: " << radius << endl;

    pcout << getMultiBlockInfo(lattice) << std::endl;

    // Switch off periodicity.
    lattice.periodicity().toggleAll(false);

    pcout << "Initilization of rho and u." << endl;
    initializeAtEquilibrium(lattice, lattice.getBoundingBox(), rho0 , u0);

    // Set NoDynamics to improve performance!
    set_nodynamics(lattice, nx, ny, nz, diameter);
        
    T rhoBar_target = 0;
    //const T mach_number = 0.2;
    //const T velocity_flow = mach_number*lattice_speed_sound;
    Array<T,3> j_target(0, 0, 0);
    T size_anechoic_buffer = 30;
    plint off_set_z = 30 + 3*diameter;
    defineAnechoicMRTBoards_limited(nx, ny, nz, lattice, size_anechoic_buffer,
      omega, j_target, j_target, j_target, j_target, j_target, j_target,
      rhoBar_target, off_set_z);
    /*defineAnechoicMRTBoards(nx, ny, nz, lattice, size_anechoic_buffer,
      omega, j_target, j_target, j_target, j_target, j_target, j_target,
      rhoBar_target);*/


    /*(MultiBlockLattice3D<T,DESCRIPTOR>& lattice, plint nx, plint ny,
    Array<plint,3> position, plint radius, plint length, plint thickness)*/
    build_duct(lattice, nx, ny, position, radius, length_duct, thickness_duct, omega);

    lattice.initialize();

    pcout << std::endl << "Voxelizing the domain." << std::endl;

    pcout << "Simulation begins" << endl;

    // Setting probes
    // half size
    plint radius_probe = (radius - 1)/sqrt(2);
    
    plint position_z_3r = position[2] + length_duct - 3*radius;
    Box3D surface_probe_3r(nx/2 - (radius_probe)/sqrt(2), 
            nx/2 + (radius_probe)/sqrt(2), 
            ny/2 - (radius_probe)/sqrt(2), 
            ny/2 + (radius_probe)/sqrt(2),
            position_z_3r, position_z_3r);
    Probe probe_3r(surface_probe_3r, fNameOut, "3r");

    plint position_z_6r = position[2] + length_duct - 6*radius;
    Box3D surface_probe_6r(nx/2 - (radius_probe)/sqrt(2), 
            nx/2 + (radius_probe)/sqrt(2), 
            ny/2 - (radius_probe)/sqrt(2), 
            ny/2 + (radius_probe)/sqrt(2),
            position_z_6r, position_z_6r);
    Probe probe_6r(surface_probe_6r, fNameOut, "6r");

    plint position_z_boca = position[2] + length_duct;
    Box3D surface_probe_boca(nx/2 - (radius_probe)/sqrt(2), 
            nx/2 + (radius_probe)/sqrt(2), 
            ny/2 - (radius_probe)/sqrt(2), 
            ny/2 + (radius_probe)/sqrt(2),
            position_z_boca, position_z_boca);
    Probe probe_boca(surface_probe_boca, fNameOut, "boca");

    std::string signal_in_string = fNameOut+"/signal_in.dat";
    char to_char_signal_in[1024];
    strcpy(to_char_signal_in, signal_in_string.c_str());
    plb_ofstream history_signal_in(to_char_signal_in);
    
    t = clock();
    std::string AllSimulationInfo_string = fNameOut + "/AllSimulationInfo.txt";
    char to_char_AllSimulationInfo[1024];
    strcpy(to_char_AllSimulationInfo, AllSimulationInfo_string.c_str());
    plb_ofstream AllSimulationInfo(to_char_AllSimulationInfo);

    std::string title = "\nTestando a classe probe.\n"; 
    AllSimulationInfo << endl
    << title << endl
    << "Dados da simulação" << endl
    << "Lattice:" << endl << endl 
    << "nx: " << nx << " ny: " << ny << " nz: " << nz << endl
    << " omega: " << omega << endl << endl
    << "Tempos: " << endl
    << "Total Time step: " << maxT << endl
    << "Discretizacao: " << radius/thickness_duct << endl
    << "Tamanho duto: " << length_duct << endl
    << "Posicao do duto: " << position[2] << endl;

    for (plint iT=0; iT<maxT; ++iT){
        if (iT <= maxT_final_source){
            plint total_signals = 20;
            T chirp_hand = get_linear_chirp_AZ(ka_max, total_signals, maxT_final_source, iT, drho, radius);
            T rho_changing = 1. + drho*sin(2*M_PI*(lattice_speed_sound/20)*iT);
            history_signal_in << setprecision(10) << chirp_hand << endl;
            set_source(lattice, position, chirp_hand, u0, radius, radius_intern, nx, ny);
        }else{
            set_source(lattice, position, rho0, u0, radius, radius_intern, nx, ny);
        }

        /*T rho_changing = 1. + (drho*100)*sin(2*M_PI*(lattice_speed_sound/20)*iT);
        history_signal_in << setprecision(10) << rho_changing << endl;
        Box3D test_source(nx/2, nx/2, ny/2, ny/2, boca_duct, boca_duct);
        initializeAtEquilibrium(lattice, test_source, rho_changing, u0);*/

        if (iT % 10 == 0 && iT>0) {
            pcout << "Iteration " << iT << endl;
        }

        if (iT % 10 == 0) {
            //writeGifs(lattice,iT);
            writeVTK(lattice, iT);
        }

        // extract values of pressure and velocities
        probe_3r.save_point(lattice, rho0, cs2);
        probe_6r.save_point(lattice, rho0, cs2);
        probe_boca.save_point(lattice, rho0, cs2);

        //lattice.collideAndStream();
    }

    t = (clock() - t)/CLOCKS_PER_SEC;
    AllSimulationInfo << endl << "Execution time: " << t << " segundos" << endl;

    pcout << "End of simulation at iteration " << endl;
}